#pragma once

#include <Core/Reflect.h>
#include <Core/Components/Component.h>

class {% componentName %} : public Component
{
public:
    {% componentName %}(
        {% for p in properties %}
            {% ifeq p.type BOOL %}
                bool {% p.name %}
            {% endif %}
            {% ifeq p.type DOUBLE %}
                double {% p.name %}
            {% endif %}
            {% ifeq p.type STRING %}
                std::string {% p.name %}
            {% endif %}
            {% ifeq #isLast false %}
                ,
            {% endif %}
        {% endfor %}
    ) : 
        {% for p in properties %}
            {% p.name %}({% p.name %})
            {% ifeq #isLast false %}
                ,
            {% endif %}
        {% endfor %}
    {
        receiveTicks = true;
        type = "{% componentName %}";
    }

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args = {}) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    {% for p in properties %}
        {% ifeq p.type BOOL %}
            bool {% p.name %};
        {% endif %}
        {% ifeq p.type DOUBLE %}
            double {% p.name %};
        {% endif %}
        {% ifeq p.type STRING %}
            std::string {% p.name %};
        {% endif %}
    {% endfor %}
};