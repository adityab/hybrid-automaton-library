///////////////////////////////////////////////////////////
// Macros for registering controllers with HybridAutomaton

// Required to enable deserialization of this controller.
// Invoke this in the header of your controller.
//
// It will create (amongst other things) a factory method which is passed
// a ha::DescriptionTreeNode::Ptr and a ha::System::Ptr. 
// You are free to choose the name of these parameters and pass
// them as arguments for the macro.
//
// Inside the factory method you should create a new instance of your controller
// and pass to it the system and deserialize it from the
// passed node (both things are in principle optional, but the standard
// use case).
//
// Attention: Be sure to make this factory method public!
//
// Example:
//   public:
//   HA_CONTROLLER_INSTANCE(node, system) {
//		YourController::Ptr mc(new YourController);
//		mc->setSystem(system);
//		mc->deserialize(node);
//		return mc;
//   }
//
// Why do we do it like this? It could be that YourController requires
// special arguments in the constructor, which you can pass here.
//
// Internally, this factory method will be called by HybridAutomaton when
// deserializing a HybridAutomaton.
//
// @see HA_CONTROLLER_REGISTER
#define HA_CONTROLLER_INSTANCE(NODE_NAME, SYSTEM_NAME) \
	class Initializer {\
	public:\
	Initializer();\
	};\
	static Initializer initializer;\
	static Controller::Ptr instance(::ha::DescriptionTreeNode::Ptr NODE_NAME, ::ha::System::Ptr SYSTEM_NAME)

// Registration method for registering your controller with the HybridAutomaton.
//
// The first parameter specifies the name that will be used 
// in the serialization, the second is the class name
// of your controller.
// Invoke this in the cpp file of your controller.
// Example:
//   HA_CONTROLLER_REGISTER("JointController", rlabJointController)
#define HA_CONTROLLER_REGISTER(STR, NAME) NAME::Initializer initializer;\
	NAME::Initializer::Initializer() { ha::HybridAutomaton::registerController(STR, &NAME::instance); }


///////////////////////////////////////////////////////////
// Macros for registering control sets with HybridAutomaton

// Required to enable deserialization of this controller.
// Invoke this in the header of your control set.
// Example:
//   HA_CONTROLSET_REGISTER_HEADER()
#define HA_CONTROLSET_REGISTER_HEADER()\
	class Initializer {\
	public:\
	Initializer();\
	};\
	static Initializer initializer;\

// The first parameter specifies the name that will be used 
// in the serialization, the second is the class name
// of your control set.
// Invoke this in the cpp file of your control set.
// Example:
//   HA_CONTROLSET_REGISTER_CPP("MyControlSet", rlabMyControlSet)
#define HA_CONTROLSET_REGISTER_CPP(STR, NAME) NAME::Initializer initializer;\
	NAME::Initializer::Initializer() { ha::HybridAutomaton::registerControlSet(STR, &NAME::instance); }

