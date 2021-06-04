#include "DefaultTemplateRenderer.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string_view>
#include <variant>

#include "Logging/Logger.h"
#include "Util/FileSlurper.h"
#include "Util/Strings.h"

static auto const logger = Logger::Create("DefaultTemplateRenderer");

struct TemplateRenderContext {
    std::filesystem::path & templatePath;
    std::stringstream & ss;
    SerializedObject & ctx;
};

struct VariableExpr {
    std::string variableName;
};

struct PropertyExpr;
using AccessExpr = std::variant<VariableExpr, std::shared_ptr<PropertyExpr>>;

struct ForInExpr {
    std::string iterVariableName;
    std::string arrayVariableName;
};

struct EndForExpr {
};

struct IfExpr {
    AccessExpr accessExpr;
    std::string equalTo;
};

struct EndIfExpr {
};

struct PropertyExpr {
    std::string variableName;
    AccessExpr innerExpr;
};

enum ExprType { VARIABLE = 0, FOR_IN = 1, END_FOR = 2, IF = 3, END_IF = 4, PROPERTY = 5 };
using TemplateExpr = std::variant<VariableExpr, ForInExpr, EndForExpr, IfExpr, EndIfExpr, PropertyExpr>;

struct FindExprResult {
    bool hasParseError;
    bool hasExpr;
    std::optional<TemplateExpr> expr;
    size_t startPos;
    size_t endPos;
};

AccessExpr ParsePropertyExpr_r(std::vector<std::string> const & split)
{
    assert(split.size() != 0);
    if (split.size() == 1) {
        return VariableExpr(split[0]);
    }
    std::vector<std::string> split2(split.begin() + 1, split.end());
    return std::make_shared<PropertyExpr>(split[0], ParsePropertyExpr_r(split2));
}

std::optional<TemplateExpr> ParseExpr(std::string str)
{
    auto split = Strings::Split(Strings::Trim(str));
    if (split.size() == 1) {
        auto splitProperty = Strings::Split(split[0], '.');
        if (splitProperty.size() > 1) {
            auto accessExpr = ParsePropertyExpr_r(splitProperty);
            if (accessExpr.index() == 0) {
                return std::get<0>(accessExpr);
            } else if (accessExpr.index() == 1) {
                return *std::get<1>(accessExpr);
            } else {
                return std::nullopt;
            }
        } else if (split[0] == "endfor") {
            return EndForExpr();
        } else if (split[0] == "endif") {
            return EndIfExpr();
        }
        return VariableExpr(split[0]);
    } else if (split.size() == 3) {
        if (split[0] == "ifeq") {
            auto splitProperty = Strings::Split(split[1], '.');
            auto accessExpr = ParsePropertyExpr_r(splitProperty);
            return IfExpr{.accessExpr = accessExpr, .equalTo = split[2]};
        }
    } else if (split.size() == 4) {
        if (split[0] == "for" && split[2] == "in") {
            return ForInExpr{
                .iterVariableName = split[1],
                .arrayVariableName = split[3],
            };
        }
    }
    logger->Warnf("Failed to parse expression='%s'", str.c_str());
    return std::nullopt;
}

bool PrintVariableExpression(std::stringstream & ss, SerializedObject & ctx, VariableExpr variableExpr)
{
    auto variableName = variableExpr.variableName;
    auto valueType = ctx.GetType(variableName);
    if (!valueType.has_value()) {
        logger->Warnf("template rendering error: variable '%s' does not exist in context", variableName.c_str());
        return false;
    }
    switch (valueType.value()) {
    case SerializedValueType::BOOL: {
        ss << (ctx.GetBool(variableName).value() ? "true" : "false");
        return true;
    }
    case SerializedValueType::DOUBLE: {
        ss << ctx.GetNumber(variableName).value();
        return true;
    }
    case SerializedValueType::STRING: {
        ss << ctx.GetString(variableName).value();
        return true;
    }
    case SerializedValueType::ARRAY:
        logger->Warnf("template rendering error: variable '%s' has unsupported type ARRAY", variableName.c_str());
        return false;
    case SerializedValueType::OBJECT:
        logger->Warnf("template rendering error: variable '%s' has unsupported type OBJECT", variableName.c_str());
        return false;
    default:
        logger->Warnf("template rendering error: variable '%s' has unknown SerializedValueType %d",
                      variableName.c_str(),
                      valueType.value());
    }
    return false;
}

FindExprResult FindExpr(std::string_view str)
{
    size_t startIndex = str.find("{%");
    if (startIndex == str.npos) {
        return {
            .hasParseError = false,
            .hasExpr = false,
        };
    }
    size_t endIndex = str.find("%}", startIndex);
    if (endIndex == str.npos) {
        return {
            .hasParseError = true,
        };
    }
    std::string_view sliceToEnd = str.substr(startIndex + 2, endIndex - startIndex - 2);
    auto exprOpt = ParseExpr(std::string(sliceToEnd));
    if (!exprOpt.has_value()) {
        return {.hasParseError = true};
    }
    return {
        .hasParseError = false, .hasExpr = true, .expr = exprOpt.value(), .startPos = startIndex, .endPos = endIndex};
}

std::optional<SerializedValue> EvaluateAccessExpr_r(SerializedObject & obj, AccessExpr expr)
{
    if (expr.index() == 0) {
        auto variableExpr = std::get<0>(expr);
        auto variableName = variableExpr.variableName;
        auto valueType = obj.GetType(variableName);
        if (!valueType.has_value()) {
            logger->Warnf("template rendering error: variable '%s' does not exist in context", variableName.c_str());
            return false;
        }
        switch (valueType.value()) {
        case SerializedValueType::BOOL: {
            return obj.GetBool(variableName).value();
        }
        case SerializedValueType::DOUBLE: {
            return obj.GetNumber(variableName).value();
        }
        case SerializedValueType::STRING: {
            return obj.GetString(variableName).value();
        }
        case SerializedValueType::ARRAY:
            return obj.GetArray(variableName).value();
        case SerializedValueType::OBJECT:
            return obj.GetObject(variableName).value();
        default:
            logger->Warnf("template rendering error: variable '%s' has unknown SerializedValueType %d",
                          variableName.c_str(),
                          valueType.value());
        }
        return std::nullopt;
    } else if (expr.index() == 1) {
        auto propertyExpr = std::get<1>(expr);
        auto innerObj = obj.GetObject(propertyExpr->variableName);
        if (!innerObj.has_value()) {
            return std::nullopt;
        }
        return EvaluateAccessExpr_r(innerObj.value(), propertyExpr->innerExpr);
    } else {
        return std::nullopt;
    }
}

bool RenderPropertyExpr_r(std::stringstream & ss, SerializedObject & obj, PropertyExpr & p)
{
    auto innerObjOpt = obj.GetObject(p.variableName);
    if (!innerObjOpt.has_value()) {
        logger->Errorf("template rendering error: '%s' property is missing or is not an object", p.variableName);
        return false;
    }
    if (p.innerExpr.index() == 0) {
        return PrintVariableExpression(ss, innerObjOpt.value(), std::get<0>(p.innerExpr));
    } else {
        return RenderPropertyExpr_r(ss, innerObjOpt.value(), *std::get<1>(p.innerExpr));
    }
}

bool RenderTemplate_r(TemplateRenderContext & ctx, std::string_view templateString)
{
    while (true) {
        if (templateString.empty()) {
            return true;
        }
        FindExprResult findExprResult = FindExpr(templateString);
        if (findExprResult.hasParseError) {
            return false;
        }
        if (findExprResult.hasExpr) {
            ctx.ss << templateString.substr(0, findExprResult.startPos);
            auto expr = findExprResult.expr.value();
            if (expr.index() == ExprType::VARIABLE) {
                auto variableExpr = std::get<VariableExpr>(expr);
                if (!PrintVariableExpression(ctx.ss, ctx.ctx, variableExpr)) {
                    logger->Errorf("%ls: template rendering error: failed to print variable expression",
                                   ctx.templatePath.c_str());
                    return false;
                }
                templateString = templateString.substr(findExprResult.endPos + 2);
            } else if (expr.index() == ExprType::FOR_IN) {
                auto forInExpr = std::get<1>(expr);
                auto arrayOpt = ctx.ctx.GetArray(forInExpr.arrayVariableName);
                if (!arrayOpt.has_value()) {
                    logger->Errorf("%ls: template rendering error: variable '%s' is not an array",
                                   ctx.templatePath.c_str(),
                                   forInExpr.arrayVariableName.c_str());
                    return false;
                }
                std::string_view remainderSlice = templateString.substr(findExprResult.endPos + 2);
                size_t loopEndPos = 0;
                bool hasEndFor = false;
                while (!remainderSlice.empty()) {
                    FindExprResult innerResult = FindExpr(remainderSlice);
                    if (innerResult.hasParseError) {
                        logger->Errorf("%ls: template rendering error: error when parsing content of loop",
                                       ctx.templatePath.c_str());
                        return false;
                    }
                    if (innerResult.expr.has_value() && innerResult.expr.value().index() == ExprType::END_FOR) {
                        loopEndPos += innerResult.startPos;
                        hasEndFor = true;
                        break;
                    }
                    remainderSlice = remainderSlice.substr(innerResult.endPos + 2);
                    loopEndPos += innerResult.endPos + 2;
                }
                if (!hasEndFor) {
                    logger->Errorf("%ls: template rendering error: no matching endfor found", ctx.templatePath.c_str());
                    return false;
                }

                for (size_t i = 0; i < arrayOpt.value().size(); ++i) {
                    bool isLast = i == arrayOpt.value().size() - 1;
                    SerializedObject innerObj = ctx.ctx.ToBuilder()
                                                    .WithValue(forInExpr.iterVariableName, arrayOpt.value()[i])
                                                    .WithBool("#isLast", isLast)
                                                    .Build();
                    TemplateRenderContext innerCtx{.templatePath = ctx.templatePath, .ss = ctx.ss, .ctx = innerObj};
                    bool success =
                        RenderTemplate_r(innerCtx, templateString.substr(findExprResult.endPos + 2, loopEndPos));
                    if (!success) {
                        return false;
                    }
                }
                templateString = templateString.substr(findExprResult.endPos + 2 + loopEndPos);
                FindExprResult endFor = FindExpr(templateString);
                if (endFor.hasParseError || !endFor.expr.has_value() ||
                    endFor.expr.value().index() != ExprType::END_FOR) {
                    return false;
                }
                templateString = templateString.substr(endFor.endPos + 2);
            } else if (expr.index() == ExprType::IF) {
                // TODO:
                auto ifExpr = std::get<ExprType::IF>(expr);
                auto variableValue = EvaluateAccessExpr_r(ctx.ctx, ifExpr.accessExpr);
                if (!variableValue.has_value()) {
                    return false;
                }

                std::optional<bool> boolOpt = std::nullopt;
                if (ifExpr.equalTo == "true") {
                    boolOpt = true;
                } else if (ifExpr.equalTo == "false") {
                    boolOpt = false;
                }
                auto doubleOpt = Strings::Strtod(ifExpr.equalTo);

                std::string_view remainderSlice = templateString.substr(findExprResult.endPos + 2);
                size_t ifEndPos = 0;
                bool hasEndIf = false;
                while (!remainderSlice.empty()) {
                    FindExprResult innerResult = FindExpr(remainderSlice);
                    if (innerResult.hasParseError) {
                        logger->Errorf("%ls: template rendering error: error when parsing content of loop",
                                       ctx.templatePath.c_str());
                        return false;
                    }
                    if (innerResult.expr.has_value() && innerResult.expr.value().index() == ExprType::END_IF) {
                        ifEndPos += innerResult.startPos;
                        hasEndIf = true;
                        break;
                    }
                    remainderSlice = remainderSlice.substr(innerResult.endPos + 2);
                    ifEndPos += innerResult.endPos + 2;
                }
                if (!hasEndIf) {
                    logger->Errorf("%ls: template rendering error: no matching endfor found", ctx.templatePath.c_str());
                    return false;
                }

                bool areEqual = false;
                if (boolOpt.has_value() && variableValue.value().GetType() == SerializedValueType::BOOL) {
                    auto val = std::get<bool>(variableValue.value());
                    areEqual = val == boolOpt.value();
                } else if (doubleOpt.has_value() && variableValue.value().GetType() == SerializedValueType::DOUBLE) {
                    auto val = std::get<double>(variableValue.value());
                    areEqual = val == doubleOpt.value();
                } else if (variableValue.value().GetType() == SerializedValueType::STRING) {
                    auto val = std::get<std::string>(variableValue.value());
                    areEqual = val == ifExpr.equalTo;
                }

                if (areEqual) {
                    bool success = RenderTemplate_r(ctx, templateString.substr(findExprResult.endPos + 2, ifEndPos));
                    if (!success) {
                        return false;
                    }
                }
                templateString = templateString.substr(findExprResult.endPos + 2 + ifEndPos);
                FindExprResult endFor = FindExpr(templateString);
                if (endFor.hasParseError || !endFor.expr.has_value() ||
                    endFor.expr.value().index() != ExprType::END_IF) {
                    return false;
                }
                templateString = templateString.substr(endFor.endPos + 2);
            } else if (expr.index() == ExprType::PROPERTY) {
                auto propertyExpr = std::get<ExprType::PROPERTY>(expr);
                bool success = RenderPropertyExpr_r(ctx.ss, ctx.ctx, propertyExpr);
                if (!success) {
                    return false;
                }
                templateString = templateString.substr(findExprResult.endPos + 2);
            } else {
                logger->Errorf("%ls: template rendering error: unknown expression type %zu",
                               ctx.templatePath.c_str(),
                               expr.index());
                return false;
            }

            if (templateString.empty()) {
                break;
            }
        } else {
            ctx.ss << templateString;
            break;
        }
    }
    return true;
}

std::optional<std::string> DefaultTemplateRenderer::RenderTemplate(std::filesystem::path templatePath,
                                                                   SerializedObject ctx)
{
    if (!templatePath.has_filename()) {
        logger->Warnf("Will not render template at path=%ls because it is not a file.", templatePath.c_str());
        return std::nullopt;
    }
    std::string templateContent = fileSlurper->SlurpFile(templatePath.string());

    std::string_view templateContentSlice(templateContent);
    std::stringstream ss;
    TemplateRenderContext renderContext{.templatePath = templatePath, .ss = ss, .ctx = ctx};
    bool success = RenderTemplate_r(renderContext, templateContentSlice);
    if (!success) {
        return std::nullopt;
    }
    return ss.str();
}
