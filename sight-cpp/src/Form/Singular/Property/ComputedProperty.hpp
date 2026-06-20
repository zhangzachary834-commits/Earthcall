template <typename Owner, typename T>
class ComputedProperty : public Property {
public:
    using Getter = T (Owner::*)() const;
    using Setter = void (Owner::*)(const T&);

    // Wrap getter/setter methods instead of raw member variables.
};