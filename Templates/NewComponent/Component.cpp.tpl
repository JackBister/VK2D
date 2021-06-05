#include "{% componentName %}.h"

#include <Core/entity.h>
#include <Core/Input/Input.h>
#include <Logging/Logger.h>

static auto const logger = Logger::Create("{% componentName %}");

REFLECT_STRUCT_BEGIN({% componentName %})
{% for p in properties %}
REFLECT_STRUCT_MEMBER({% p.name %})
{% endfor %}
REFLECT_STRUCT_END()

static SerializedObjectSchema const {% componentName %}_SCHEMA =
    SerializedObjectSchema("{% componentName %}",
                           {
                            {% for p in properties %}
                                {% ifeq p.type BOOL %}
                                    SerializedPropertySchema::Required("{% p.name %}", SerializedValueType::BOOL)
                                {% endif %}
                                {% ifeq p.type DOUBLE %}
                                    SerializedPropertySchema::Required("{% p.name %}", SerializedValueType::DOUBLE)
                                {% endif %}
                                {% ifeq p.type STRING %}
                                    SerializedPropertySchema::Required("{% p.name %}", SerializedValueType::STRING)
                                {% endif %}
                                {% ifeq #isLast false %}
                                    ,
                                {% endif %}
                            {% endfor %}
                           },
                           {SerializedObjectFlag::IS_COMPONENT});

class {% componentName %}Deserializer : public Deserializer
{
    std::string constexpr GetOwner() final override { return "{% projectName %}"; }

    SerializedObjectSchema GetSchema() final override { return {% componentName %}_SCHEMA; }

    void *Deserialize(DeserializationContext *ctx, SerializedObject const &obj) final override
    {
        {% for p in properties %}
            {% ifeq p.type BOOL %}
                bool {% p.name %} = obj.GetBool("{% p.name %}").value();
            {% endif %}
            {% ifeq p.type DOUBLE %}
                double {% p.name %} = obj.GetNumber("{% p.name %}").value();
            {% endif %}
            {% ifeq p.type STRING %}
                std::string {% p.name %} = obj.GetString("{% p.name %}").value();
            {% endif %}
        {% endfor %}
        return new {% componentName %}(
            {% for p in properties %}
                {% p.name %}
                {% ifeq #isLast false %}
                    ,
                {% endif %}
            {% endfor %}
        );
    }
};

COMPONENT_IMPL({% componentName %}, new {% componentName %}Deserializer())

SerializedObject {% componentName %}::Serialize() const
{
    return SerializedObject::Builder()
        .WithString("type", this->Reflection.name)
        {% for p in properties %}
            {% ifeq p.type BOOL %}
                .WithBool("{% p.name %}", {% p.name %})
            {% endif %}
            {% ifeq p.type DOUBLE %}
                .WithNumber("{% p.name %}", {% p.name %})
            {% endif %}
            {% ifeq p.type STRING %}
                .WithString("{% p.name %}", {% p.name %})
            {% endif %}
        {% endfor %}
        .Build();
}

void {% componentName %}::OnEvent(HashedString name, EventArgs args)
{
    if (name == "BeginPlay") {
        logger.Info("Hello world!");
    } else if (name == "Tick") {
        if (Input::GetKeyDown(Keycode::KC_MOUSE_LEFT))
        {
            logger.Info("Click!");
        }
    }
}
