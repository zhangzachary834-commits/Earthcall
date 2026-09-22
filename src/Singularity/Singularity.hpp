#pragma once

/*
    DEVELOPER NOTE: 
    This setup is built for our current computational paradigms, like conventional OS systems, app design, and robotics. 
    Change these enum primitives based on the fundamental mechanical elements of different systems as needed. 
    I look forward to seeing someone write 'enum class ElementComputational { quark }' someday
*/

class Singularity {
public:

    enum class Direction {
        Input,
        InnateComputational,
        Output
    };

    enum class Modality {
        Screen,
        Sound,
        Physical,
        Symbolic,
        Joys

    };

    enum class Earthcall {
        Singularity,
        Singular,
        Relation,
        Formation,
        Person,
        Soul,
        Relationship,
        Community,
        Object,
        Law,
        Zone,
        Home,

    };

    enum class ElementInput {
        Mouse,
        Keyboard,
        MovementSensor, //for potential VR/AR/XR systems
        Microphone

    };

    enum class ElementComputational {
        CPU,
        neuron_layer,
        data_structure,
        laws_of_math_model,
        algorithm,
        GPU,
        pattern,
        flow,
        direction

    };

    // This will foundationally clash with OpenGL and especially ImGU
    // because they both are currently treated foundational screen-primitives rather than the GPU, but this can be a placeholder for more advanced systems.
    enum class ElementOutput {
        Light, // changed from 'screen' to 'light' because light is more fundamental than screens in robotics
        Speaker,
        Movement // for robotics

    };

    // hook this up to the main Event bus-handler engine for authorial management of relations between primitives. 

    // considering adding tentative primitives for, but that's already essentially OpenGL and ImGUI. 
    // Adding new primitives may complicate code later and require extensive refactoring.

    Singularity();
    ~Singularity();
};