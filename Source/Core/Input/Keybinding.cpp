#include "Keybinding.h"

#include "Serialization/Deserializable.h"
#include "Serialization/Deserializer.h"
#include "Serialization/SerializedObjectSchema.h"

static SerializedObjectSchema const KEYBINDING_SCHEMA = SerializedObjectSchema(
    "Keybinding",
    {SerializedPropertySchema::Required("name", SerializedValueType::STRING),
     SerializedPropertySchema::RequiredArray("keyCodes", SerializedValueType::STRING,
                                             SerializedPropertyFlags({StringEnumFlag(GetAllKeycodeStrings())}))});

class KeybindingDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() override { return KEYBINDING_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) override
    {
        auto name = obj.GetString("name").value();
        auto keyCodesArr = obj.GetArray("keyCodes").value();
        std::vector<Keycode> keyCodes(keyCodesArr.size());
        for (size_t i = 0; i < keyCodesArr.size(); ++i) {
            keyCodes[i] = strToKeycode[std::get<std::string>(keyCodesArr[i])];
        }
        return new Keybinding(name, keyCodes);
    }
};

DESERIALIZABLE_IMPL(Keybinding, new KeybindingDeserializer())
