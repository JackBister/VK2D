#include "Console.h"

#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

#include <imgui.h>
#include <optick/optick.h>

#include "Logging/LogAppender.h"
#include "Logging/Logger.h"
#include "Util/Strings.h"

static const auto logger = Logger::Create("Console");

const size_t COMMAND_HISTORY_SIZE = 256;
const size_t INPUT_BUFFER_SIZE = 256;
const size_t MAX_LINES = 1024;

namespace Console
{
static void AutoComplete(ImGuiInputTextCallbackData *);
static void AutoCompleteHistory(ImGuiInputTextCallbackData *);
static void Execute(std::string const & command);
static int TextEditCallback(ImGuiInputTextCallbackData *);

std::deque<std::string> commandHistory;
size_t commandHistoryPos = 0;
std::unordered_map<std::string, CommandDefinition> commands;
char inputBuffer[INPUT_BUFFER_SIZE] = {0};
std::deque<LogMessage> lines;
RenderSystem * renderSystem = nullptr;

bool autoScroll = true;
bool isConsoleOpen = false;
bool scrollToBottom = false;

std::shared_ptr<LogAppender> GetAppender()
{
    class ConsoleLogAppender : public LogAppender
    {
    protected:
        virtual void AppendImpl(LogMessage const & message) final override
        {
            std::lock_guard<std::mutex> lock(guard);
            lines.push_back(message);
            if (lines.size() > MAX_LINES) {
                lines.pop_front();
            }
        }

    private:
        std::mutex guard;
    };
    return std::make_shared<ConsoleLogAppender>();
}

void Init(RenderSystem * inRenderSystem)
{
    renderSystem = inRenderSystem;

    CommandDefinition clearCommand("clear", "clear - Clears the console.", 0, [](auto args) { lines.clear(); });
    RegisterCommand(clearCommand);

    CommandDefinition docCommand(
        "doc", "doc <command> - Returns the docString for the given command.", 1, [](auto args) {
            auto commandName = args[0];
            if (commands.find(commandName) == commands.end()) {
                logger->Errorf("Command '%s' not found", commandName.c_str());
                return;
            }
            auto command = commands.at(commandName);
            logger->Infof("%s", command.GetDocString().c_str());
        });
    RegisterCommand(docCommand);

    CommandDefinition listCommand("list_commands", "list_commands - Lists all available commands.", 0, [](auto args) {
        for (auto const & command : commands) {
            logger->Infof("%s", command.first.c_str());
        }
    });
    RegisterCommand(listCommand);
}

void OnGui(int windowWidth, int windowHeight)
{
    OPTICK_EVENT();
    if (isConsoleOpen) {
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight / 3));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("Console",
                     &isConsoleOpen,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Autoscroll", &autoScroll);
        ImGui::PopStyleVar();

        ImGui::Separator();

        const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollArea", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

        for (auto line : lines) {
            auto isError = line.GetLevel() == LogLevel::ERROR || line.GetLevel() == LogLevel::SEVERE;
            if (isError) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
            }
            ImGui::TextUnformatted(line.GetMessage().c_str());
            if (isError) {
                ImGui::PopStyleColor();
            }
        }

        if (scrollToBottom || (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
            ImGui::SetScrollHereY(1.f);
        }
        scrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::Separator();

        bool hasInputtedText =
            ImGui::InputText("",
                             inputBuffer,
                             INPUT_BUFFER_SIZE,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion |
                                 ImGuiInputTextFlags_CallbackHistory,
                             &TextEditCallback);
        ImGui::SetItemDefaultFocus();
        ImGui::SetKeyboardFocusHere(-1);
        if (hasInputtedText) {
            std::string input(inputBuffer);
            memset(inputBuffer, 0, INPUT_BUFFER_SIZE);
            input = Strings::Trim(input);
            if (input.size() > 0) {
                Execute(input);
                commandHistory.push_back(input);
                if (commandHistory.size() > COMMAND_HISTORY_SIZE) {
                    commandHistory.pop_front();
                }
                commandHistoryPos = commandHistory.size();
                scrollToBottom = true;
            }
        }

        if (hasInputtedText) {
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::End();
    }
}

void RegisterCommand(CommandDefinition const & definition)
{
    commands.insert_or_assign(definition.GetName(), definition);
}

void ToggleVisible()
{
    isConsoleOpen = !isConsoleOpen;
}

static void Execute(std::string const & input)
{
    auto split = Strings::Split(input);
    if (split.size() == 0) {
        // shouldn't happen since we trimmed and checked string length already
        return;
    }

    auto commandName = split[0];
    if (commands.find(commandName) == commands.end()) {
        logger->Errorf("Command '%s' not found", commandName.c_str());
        return;
    }

    auto command = commands.at(commandName);
    if (command.GetNumParameters() != split.size() - 1) {
        logger->Errorf("Wrong number of arguments for command '%s', requires %d arguments but got %d",
                       commandName.c_str(),
                       command.GetNumParameters(),
                       split.size() - 1);
        return;
    }

    auto arguments = split.size() == 1 ? std::vector<std::string>() : std::vector(split.begin() + 1, split.end());
    command.Execute(arguments);
}

static int TextEditCallback(ImGuiInputTextCallbackData * data)
{
    switch (data->EventFlag) {
    case ImGuiInputTextFlags_CallbackCompletion:
        AutoComplete(data);
        break;
    case ImGuiInputTextFlags_CallbackHistory:
        AutoCompleteHistory(data);
        break;
    }
    return 0;
}

static void AutoComplete(ImGuiInputTextCallbackData * data)
{
    char const * wordEnd = data->Buf + data->CursorPos;
    char const * wordStart = wordEnd;
    while (wordStart > data->Buf) {
        char const c = wordStart[-1];
        if (c == ' ' || c == '\t') {
            break;
        }
        wordStart--;
    }
    std::string currentInput(wordStart, wordEnd);
    if (Strings::ContainsWhitespace(currentInput)) {
        return;
    }

    std::vector<std::string> candidates;
    for (auto kv : commands) {
        auto commandName = kv.first;

        if (commandName.find(currentInput) == 0) {
            candidates.push_back(commandName);
        }
    }

    if (candidates.size() == 1) {
        data->DeleteChars(wordStart - data->Buf, wordEnd - wordStart);
        data->InsertChars(data->CursorPos, candidates[0].c_str());
        data->InsertChars(data->CursorPos, " ");
    } else if (candidates.size() > 1) {
        size_t commonMatchLength = currentInput.size();

        for (auto & c : candidates) {
            logger->Infof("%s", c.c_str());
        }

        while (true) {
            if (candidates[0].size() == commonMatchLength) {
                break;
            }
            bool allMatch = true;
            char nextChar = candidates[0][commonMatchLength];
            for (auto candidate : candidates) {
                if (candidate[commonMatchLength] != nextChar) {
                    allMatch = false;
                }
            }
            if (!allMatch) {
                break;
            }
            commonMatchLength++;
        }

        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, candidates[0].substr(0, commonMatchLength).c_str());
    }
}

static void AutoCompleteHistory(ImGuiInputTextCallbackData * data)
{
    const int previousPos = commandHistoryPos;
    if (data->EventKey == ImGuiKey_UpArrow) {
        if (commandHistoryPos == 0) {
            commandHistoryPos = commandHistory.size() - 1;
        } else {
            commandHistoryPos--;
        }
    } else if (data->EventKey == ImGuiKey_DownArrow) {
        if (commandHistoryPos == commandHistory.size() - 1) {
            commandHistoryPos = 0;
        } else {
            commandHistoryPos++;
        }
    }

    if (commandHistoryPos >= commandHistory.size()) {
        commandHistoryPos = 0;
    }

    if (commandHistoryPos != previousPos && commandHistory.size() > 0) {
        auto historyEntry = commandHistory[commandHistoryPos];
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, historyEntry.c_str());
    }
}
};
