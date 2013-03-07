#include "XMLDeserializer.h"

#include "CSpaceMilestone.h"
#include "OpSpaceMilestone.h"

#include "rControlalgorithm\rControlalgorithm.h"
#include "rxControlSDK\rxControlSDK.h"

#include "FeatureAttractorController.h"
#include "SubdisplacementController.h"
#include "ObstacleAvoidanceController.h"
#include "JointBlackBoardController.h"
#include "SingularityAvoidanceController.h"
#include "JointLimitAvoidanceControllerOnDemand.h"
#include "OpSpaceSingularityAvoidanceController.h"
#include "ReInterpolatedJointImpedanceController.h"

#include "NakamuraControlSet.h"
#include "InterpolatedSetPointDisplacementController.h"
#include "InterpolatedSetPointOrientationController.h"
#include "PressureDisplacementController.h"
#include "PressureOrientationController.h"

std::map<std::string, ControllerType> XMLDeserializer::controller_map_ = XMLDeserializer::createControllerMapping();

std::string XMLDeserializer::wstring2string(const std::wstring& wstr)
{
	return std::string(wstr.begin(), wstr.end());
}

std::wstring XMLDeserializer::string2wstring(const std::string& str)
{
	return std::wstring(str.begin(), str.end());
}

/**
* Deserializing Functions
*/

template<class T>
T deserializeElement(TiXmlElement * xml_element, const char * field_name)
{
	T return_value;

	if(!xml_element->Attribute(field_name, &return_value))
	{
		std::string exception_str = std::string("[deserializeElement] ERROR: Attribute ") + std::string(field_name) + std::string(" was not found in XML element.");
		throw exception_str;
	}
	
	return return_value;
}

template<class T>
T deserializeElement(TiXmlElement * xml_element, const char * field_name, T default_value)
{
	T return_value = default_value;
	xml_element->Attribute(field_name, &return_value);
	return return_value;
}

bool deserializeBoolean(TiXmlElement * xml_element, const char * field_name, bool default_value)
{
	const char* value = xml_element->Attribute(field_name);

	if (value)
	{
		if (strcmp(value, "true") == 0)
		{
			return true;
		}
		else if (strcmp(value, "false") == 0)
		{
			return false;
		}
	}
	return default_value;
}

std::string deserializeString(TiXmlElement * xml_element, const char * field_name, const std::string& default_value)
{
	const char* ret_char_ptr = xml_element->Attribute(field_name);

	if(ret_char_ptr)
		return std::string(ret_char_ptr);
	
	return default_value;
}

std::string deserializeString(TiXmlElement * xml_element, const char * field_name, bool error_if_not_found)
{
	const char* ret_char_ptr = xml_element->Attribute(field_name);
	if(ret_char_ptr)
	{
		return std::string(ret_char_ptr);
	}else{
		if(error_if_not_found)
		{
			std::string exception_str = std::string("[deserializeString] ERROR: Attribute ") 
				+ std::string(field_name) + std::string(" was not found in XML element.");
			throw exception_str;
		}
	}
	return std::string();
}

dVector deserializeDVector(TiXmlElement * xml_element, const char * field_name)
{
	std::stringstream ss(deserializeString(xml_element, field_name, false));
	dVector v;
	v.all(-1.0);
	double value = -1.0;
	while(ss >> value)
	{	
		v.expand(1, value);
	}
	return v;
}

Rotation deserializeRotation(TiXmlElement * xml_element, const char * field_name, const Rotation& default_value)
{
	std::string rotation = deserializeString(xml_element, field_name, std::string(""));

	if (rotation.empty())
		return default_value;

	return XMLDeserializer::string2rotation(rotation, default_value);
}

Displacement deserializeDisplacement(TiXmlElement * xml_element, const char * field_name, const Displacement& default_value)
{
	dVector v = deserializeDVector(xml_element, field_name);
	if (v.size() != 3)
		return default_value;

	return v;
}

rxBody* deserializeBody(rxSystem* robot, TiXmlElement * xml_element, const char * field_name, rxBody* default_value)
{
	std::string name = deserializeString(xml_element, field_name, std::string(""));
	if (name.empty())
		return default_value;

	//First we query for user defined bodies
	rxBody* body = robot->getUCSBody(XMLDeserializer::string2wstring(name), HTransform());
	if (body != NULL)
		return body;

	//Now query for bodies in the model
	body = robot->findBody(XMLDeserializer::string2wstring(name));
	if(body != NULL)
		return body;

	std::string exception_str = std::string("[deserializeBody] ERROR: Body ") 
		+ name + std::string(" was not found in aml");
	throw exception_str;
	return NULL;


}

template<class T>
std::vector<T> deserializeStdVector(TiXmlElement * xml_element, const char * field_name)
{
	std::vector<T> ret_vector;
	std::stringstream vector_ss = std::stringstream(deserializeString(xml_element, field_name, false));
	double vector_value = -1.0;
	while ((vector_ss >> vector_value))
	{
		ret_vector.push_back((T)vector_value);
	}
	return ret_vector;
}

ViaPointBase * deserializeViaPoint(TiXmlElement * xml_element, ControllerType type_of_controller, int controller_dimension)
{
	ViaPointBase * return_value;
	double via_point_time = deserializeElement<double>(xml_element, "time");
	int via_point_type = deserializeElement<int>(xml_element, "type");
	bool via_point_reuse_bool = deserializeBoolean(xml_element, "reuse");
	switch( type_of_controller.first )
	{
	case rxController::eControlType_Joint:	
		{
			std::stringstream via_point_ss = std::stringstream(deserializeString(xml_element, "dVector"));
			double via_point_value = -1.0;
			dVector via_point_dVector;
			std::string value_str;
			while(getline(via_point_ss, value_str, ',')){
				via_point_dVector.expand(1,atof(value_str.c_str()));
			}
			// NOTE (Roberto): This older version works when the values are separated with white spaces instead of commas
			//for(int i=0; i<controller_dimension; i++)
			//{
			//	via_point_ss >> via_point_value;
			//	via_point_dVector.expand(1,via_point_value);
			//}
			return_value = new ViaPointdVector(via_point_time, via_point_type, via_point_reuse_bool, via_point_dVector);
			break;
		}
	case rxController::eControlType_Displacement:	
		{
			std::stringstream via_point_ss = std::stringstream(deserializeString(xml_element, "Vector3D"));
			double via_point_value = -1.0;
			Vector3D via_point_Vector3D;
			for(int i=0; i<controller_dimension; i++)
			{
				via_point_ss >> via_point_value;
				via_point_Vector3D.expand(1,via_point_value);
			}
			return_value = new ViaPointVector3D(via_point_time, via_point_type, via_point_reuse_bool, via_point_Vector3D);
			break;
		}
	case rxController::eControlType_Orientation:	
		{
			std::stringstream via_point_ss = std::stringstream(deserializeString(xml_element, "R"));
			double via_point_value = -1.0;
			std::vector<double> via_point_values;
			for(int i=0; i<9; i++)
			{
				via_point_ss >> via_point_value;
				via_point_values.push_back(via_point_value);
			}
			Rotation via_point_Rotation(via_point_values[0], via_point_values[1],via_point_values[2],via_point_values[3],
				via_point_values[4],via_point_values[5],via_point_values[6],via_point_values[7],via_point_values[8]);
			return_value = new ViaPointRotation(via_point_time, via_point_type, via_point_reuse_bool, via_point_Rotation);
			break;
		}
	case rxController::eControlType_HTransform:	
		{
			std::stringstream via_point_ss = std::stringstream(deserializeString(xml_element, "R"));
			double via_point_value = -1.0;
			std::vector<double> via_point_values;
			for(int i=0; i<9; i++)
			{
				via_point_ss >> via_point_value;
				via_point_values.push_back(via_point_value);
			}
			Rotation via_point_Rotation(via_point_values[0], via_point_values[1],via_point_values[2],via_point_values[3],
				via_point_values[4],via_point_values[5],via_point_values[6],via_point_values[7],via_point_values[8]);

			std::stringstream via_point_ss2 = std::stringstream(deserializeString(xml_element, "r"));
			via_point_values.clear();
			for(int i=0; i<9; i++)
			{
				via_point_ss2 >> via_point_value;
				via_point_values.push_back(via_point_value);
			}
			Displacement via_point_Displacement(via_point_values[0], via_point_values[1],via_point_values[2]);		

			HTransform via_point_HTransform(via_point_Rotation, via_point_Displacement);

			return_value = new ViaPointHTransform(via_point_time, via_point_type, via_point_reuse_bool, via_point_HTransform);
			break;
		}
	default:
		std::string exception_str = std::string("[deserializeViaPoint] ERROR: Unknown type of Controller group.");
		throw exception_str;
		break;
	}
	return return_value;
}

Rotation XMLDeserializer::string2rotation(const std::string& str, const Rotation& default_value)
{
	std::stringstream ss = std::stringstream(str);
	double value = -1.0;
	std::vector<double> values;
	while(ss >> value)
	{
		values.push_back(value);
	}
	assert(values.size() == 3 || values.size() == 4 || values.size() == 9);

	// decide whether euler, quaternion, or matrix
	switch (values.size())
	{
	case 3: return Rotation(values[0], values[1], values[2]);
	case 4: return Rotation(values[0], values[1], values[2], values[3]); // attention: w, x, y, z
	case 9: return Rotation(values[0], values[1], values[2],
							values[3], values[4], values[5],
							values[6], values[7], values[8]);
	}

	return default_value;
}

std::string colon2space(std::string text)
{
	for(unsigned int i = 0; i < text.length(); i++)
	{
		if( text[i] == ',' )
			text[i] = ' ';
	}
	return text;
}




/**
* The XMLDeserializer is the object that reads all the information from the xml string and deserializes it, creating
* all required objects (Milestones, MotionBehaviours and the HybridAutomaton).
* The idea is to deserialize all the data here (thus, read it from the xml string and convert it into the corresponding type)
* and then call the suitable constructor.
*/

XMLDeserializer::XMLDeserializer()
{
}

XMLDeserializer::~XMLDeserializer()
{
}

HybridAutomaton* XMLDeserializer::createHybridAutomaton(const std::string& xml_string, rxSystem* robot, double dT)
{
	/*ofstream file;
	file.open("hybrid_automaton.txt");
	file << xml_string;
	file.close();*/
	HybridAutomaton* automaton = new HybridAutomaton();

	// Create the DOM-model
	TiXmlDocument document;
	const char* ret_val = document.Parse(xml_string.c_str());
	if(ret_val == NULL && document.Error()) 	{
		std::cout << "ERROR CODE: " <<  document.ErrorId() << std::endl;
		std::cout << document.ErrorDesc() << std::endl;
		throw std::string("[XMLDeserializer::createHybridAutomaton] ERROR: Parsing the string.");
		delete automaton;
		return NULL;
	}
	TiXmlHandle docHandle(&document);

	// Find the first (there should be the only one) HybridAutomaton element in the base document
	TiXmlElement* ha_element = docHandle.FirstChild("HybridAutomaton").Element();

	// Check if the HybridAutomaton element was found
	if (ha_element == NULL) {
		throw std::string("[XMLDeserializer::createHybridAutomaton] ERROR: Child Element \"HybridAutomaton\" not found in XML element docHandle.");
		delete automaton;
		return NULL;
	}

	std::string start_node = std::string(ha_element->Attribute("InitialMilestone"));
	if(start_node == std::string("")) {
		std::cout << "[XMLDeserializer::createHybridAutomaton] ERROR: Attribute \"InitialMilestone\" not found in XML element hsElement." << std::endl;
		delete automaton;
		return NULL;
	}

	// Print out a message with the general properties of the new HybridAutomaton.
	std::cout << "[XMLDeserializer::createHybridAutomaton] INFO: Creating hybrid system '" << ha_element->Attribute("Name") << "'. Initial Milestone: " << start_node << std::endl;

	// Read the data of the nodes-milestones and create them
	for (TiXmlElement* mst_element = ha_element->FirstChildElement("Milestone"); mst_element != 0; mst_element = mst_element->NextSiblingElement("Milestone")) {
		Milestone* mst;
		std::string mst_type(mst_element->Attribute("type"));
		if(mst_type == "CSpace")
		{
			mst = XMLDeserializer::createCSpaceMilestone(mst_element, robot, dT);
		}
		else if(mst_type == "OpSpace")
		{
			mst = XMLDeserializer::createOpSpaceMilestone(mst_element, robot, dT);
		}
		else if(mst_type == "CSpaceBlackBoard")
		{
			mst = XMLDeserializer::createCSpaceBlackBoardMilestone(mst_element, robot, dT);
		}
		else
		{
			throw (std::string("[XMLDeserializer::createHybridAutomaton] ERROR: Unknown type of milestone: ") + mst_type);
			delete automaton;
			return NULL;
		}
		automaton->addNode(mst);
	}

	// Set the start node-milestone
	automaton->setStartNode(automaton->getMilestoneByName(start_node));

	if (!automaton->getStartNode()) {
		throw ("[XMLDeserializer::createHybridAutomaton] ERROR: The name of the initial node '" + start_node + "' does not match with the name of any milestone.");
		delete automaton;
		return NULL;
	}

	// Read the data of the edges-motionbehaviours and create them
	for (TiXmlElement* mb_element = ha_element->FirstChildElement("MotionBehaviour"); mb_element != 0; mb_element = mb_element->NextSiblingElement("MotionBehaviour")) {
		std::string ms_parent(mb_element->Attribute("Parent"));
		std::string ms_child(mb_element->Attribute("Child"));
		if(ms_parent == std::string(""))
		{
			throw std::string("[XMLDeserializer::createHybridAutomaton] ERROR: Attribute \"Parent\" not found in XML element edgeElement.");
			delete automaton;
			return NULL;
		}

		Milestone* ms_parent_ptr = automaton->getMilestoneByName(ms_parent);

		if(ms_parent_ptr == NULL) {
			throw std::string("[XMLDeserializer::createHybridAutomaton] ERROR: The name of the parent node does not match with the name of any milestone.");
			delete automaton;
			return NULL;
		}

		if(ms_child == std::string(""))
		{
			throw std::string("[XMLDeserializer::createHybridAutomaton] ERROR: Attribute \"Child\" not found in XML element edgeElement.");
			delete automaton;
			return NULL;
		}

		Milestone* ms_child_ptr = automaton->getMilestoneByName(ms_child);

		if(ms_child_ptr == NULL)
		{
			throw ("[XMLDeserializer::createHybridAutomaton] ERROR: The name of the child node '" + ms_child + "' does not match the name of any milestone.");
			delete automaton;
			return NULL;
		}

		MotionBehaviour* mb = XMLDeserializer::createMotionBehaviour(mb_element, ms_parent_ptr, ms_child_ptr, robot, dT);
		automaton->addEdge(mb);
	}

	return automaton;
}

CSpaceMilestone* XMLDeserializer::createCSpaceMilestone(TiXmlElement* milestone_xml, rxSystem* robot, double dT)
{
	Milestone::Status mst_status = (Milestone::Status)deserializeElement<int>(milestone_xml, "status");
	std::string mst_name = deserializeString(milestone_xml, "name");
	std::vector<double> mst_configuration = deserializeStdVector<double>(milestone_xml, "value");
	std::vector<double> mst_epsilon = deserializeStdVector<double>(milestone_xml, "epsilon");
	if(mst_configuration.size()==0 || mst_epsilon.size()==0)
	{
		throw std::string("[XMLDeserializer::createCSpaceMilestone] ERROR: The milestone configuration or region of convergence (\"value\" or \"epsilon\") is not defined in the XML string.");
	}


	TiXmlElement* handle_point_set_element = milestone_xml->FirstChildElement("HandlePoints");
	std::vector<Point> mst_handle_points;
	if(handle_point_set_element != NULL)
	{
		for(TiXmlElement* handle_point_element = handle_point_set_element->FirstChildElement("HandlePoint"); handle_point_element; handle_point_element = handle_point_element->NextSiblingElement())
		{
			Point handle_point_value(-1.f,-1.f,-1.f);
			handle_point_value.x = deserializeElement<double>(handle_point_element, "x");
			handle_point_value.y = deserializeElement<double>(handle_point_element, "y");
			handle_point_value.z = deserializeElement<double>(handle_point_element, "z");
			mst_handle_points.push_back(handle_point_value);
		}
	}

	CSpaceMilestone* mst = new CSpaceMilestone(mst_name, mst_configuration, NULL, mst_epsilon, mst_status, mst_handle_points);

	TiXmlElement* mb_element = milestone_xml->FirstChildElement("MotionBehaviour");
	MotionBehaviour* mst_mb = NULL;
	if(mb_element)
	{
		mst_mb = XMLDeserializer::createMotionBehaviour(mb_element, mst, mst, robot, dT);
		mst->setMotionBehaviour(mst_mb);
	}
    
    double expLength = deserializeElement<double>(milestone_xml, "expectedLength", -1.0);
    mst->setExpectedLength(expLength);

	return mst;
}

CSpaceBlackBoardMilestone* XMLDeserializer::createCSpaceBlackBoardMilestone(TiXmlElement* milestone_xml, rxSystem* robot, double dT)
{
	Milestone::Status mst_status = (Milestone::Status)deserializeElement<int>(milestone_xml, "status");
	std::string mst_name = deserializeString(milestone_xml, "name");
	std::vector<double> mst_configuration = deserializeStdVector<double>(milestone_xml, "value");
	std::vector<double> mst_epsilon = deserializeStdVector<double>(milestone_xml, "epsilon");
	if(mst_configuration.size()==0 || mst_epsilon.size()==0)
	{
		throw std::string("[XMLDeserializer::createCSpaceMilestone] ERROR: The milestone configuration or region of convergence (\"value\" or \"epsilon\") is not defined in the XML string.");
	}

	TiXmlElement* handle_point_set_element = milestone_xml->FirstChildElement("HandlePoints");
	std::vector<Point> mst_handle_points;
	if(handle_point_set_element != NULL)
	{
		for(TiXmlElement* handle_point_element = handle_point_set_element->FirstChildElement("HandlePoint"); handle_point_element; handle_point_element = handle_point_element->NextSiblingElement())
		{
			Point handle_point_value(-1.f,-1.f,-1.f);
			handle_point_value.x = deserializeElement<double>(handle_point_element, "x");
			handle_point_value.y = deserializeElement<double>(handle_point_element, "y");
			handle_point_value.z = deserializeElement<double>(handle_point_element, "z");
			mst_handle_points.push_back(handle_point_value);
		}
	}

	CSpaceBlackBoardMilestone* mst = new CSpaceBlackBoardMilestone(mst_name, mst_configuration, NULL, mst_epsilon, mst_status, mst_handle_points);

	mst->setBlackBoardVariableName(mst->getBlackBoardVariableName());

    double expLength = deserializeElement<double>(milestone_xml, "expectedLength", -1.0);
    mst->setExpectedLength(expLength);
	
	return mst;
}

OpSpaceMilestone* XMLDeserializer::createOpSpaceMilestone(TiXmlElement* milestone_xml, rxSystem* robot, double dT)
{
	Milestone::Status mst_status = (Milestone::Status)deserializeElement<int>(milestone_xml, "status");
	std::string mst_name = deserializeString(milestone_xml, "name");
	PosiOriSelector mst_pos = (PosiOriSelector)deserializeElement<int>(milestone_xml, "PosiOriSelector", POS_AND_ORI_SELECTION);
	//std::vector<double> mst_configuration = deserializeStdVector<double>(milestone_xml, "value");
	Displacement position = deserializeDisplacement(milestone_xml, "position", Displacement());
	Rotation orientation = deserializeRotation(milestone_xml, "orientation", Rotation());
	std::vector<double> mst_epsilon = deserializeStdVector<double>(milestone_xml, "epsilon");
	if(/*mst_configuration.size()==0 ||*/ mst_epsilon.size()==0)
	{
		throw std::string("[XMLDeserializer::createOpSpaceMilestone] ERROR: The milestone configuration or region of convergence (\"value\" or \"epsilon\") is not defined in the XML string.");
	}

	TiXmlElement* handle_point_set_element = milestone_xml->FirstChildElement("HandlePoints");
	std::vector<Point> mst_handle_points;
	if(handle_point_set_element != NULL)
	{
		for(TiXmlElement* handle_point_element = handle_point_set_element->FirstChildElement("HandlePoint"); handle_point_element; handle_point_element = handle_point_element->NextSiblingElement())
		{
			Point handle_point_value(-1.f,-1.f,-1.f);
			handle_point_value.x = deserializeElement<double>(handle_point_element, "x");
			handle_point_value.y = deserializeElement<double>(handle_point_element, "y");
			handle_point_value.z = deserializeElement<double>(handle_point_element, "z");
			mst_handle_points.push_back(handle_point_value);
		}
	}

	  OpSpaceMilestone* mst = new OpSpaceMilestone(mst_name, mst_configuration, mst_pos, NULL, mst_epsilon, mst_status, mst_handle_points);
	//OpSpaceMilestone* mst = new OpSpaceMilestone(mst_name, position, orientation, mst_pos, NULL, mst_epsilon);

	TiXmlElement* mb_element = milestone_xml->FirstChildElement("MotionBehaviour");
	MotionBehaviour* mst_mb = NULL;
	if(mb_element)
	{
		mst_mb = XMLDeserializer::createMotionBehaviour(mb_element, mst, mst, robot, dT);
		mst->setMotionBehaviour(mst_mb);
	}

    double expLength = deserializeElement<double>(milestone_xml, "expectedLength", -1.0);
    mst->setExpectedLength(expLength);

	return mst;
}

MotionBehaviour* XMLDeserializer::createMotionBehaviour(TiXmlElement* motion_behaviour_xml , Milestone *dad, Milestone *son , rxSystem* robot, double dT )
{
	TiXmlElement* control_set_element = motion_behaviour_xml->FirstChildElement("ControlSet");
	if(control_set_element == NULL)
	{
		throw std::string("[XMLDeserializer::createMotionBehaviour] ERROR: ControlSet element was not found.");
	}

	std::string control_set_type = std::string(control_set_element->Attribute("type"));
	rxControlSetBase* mb_control_set = NULL;
	if(robot)
	{
		if(control_set_type == std::string("rxControlSet"))
		{		
			mb_control_set = new rxControlSet(robot, dT);
			mb_control_set->setGravity(0, 0, -GRAV_ACC);
			mb_control_set->setInverseDynamicsAlgorithm(new rxAMBSGravCompensation(robot));
			//mb_control_set->nullMotionController()->setGain(0.02,0.0,0.01);
			mb_control_set->nullMotionController()->setGain(0.0, 0.0, 0.0);
		}
		else if(control_set_type == std::string("rxTPOperationalSpaceControlSet"))
		{		
			mb_control_set = new rxTPOperationalSpaceControlSet(robot, dT);
			mb_control_set->setGravity(0, 0, -GRAV_ACC);
			//mb_control_set->setInverseDynamicsAlgorithm(new rxAMBSGravCompensation(robot));
			//mb_control_set->nullMotionController()->setGain(0.02,0.0,0.01);
			mb_control_set->nullMotionController()->setGain(10.0,1.0); // taken from ERM values...
		}
		else if(control_set_type == "NakamuraControlSet")
		{
			mb_control_set = new NakamuraControlSet(robot, dT);
			mb_control_set->setGravity(0, 0, -GRAV_ACC);
			mb_control_set->nullMotionController()->setGain(0.0, 0.0, 0.0);
		}
		else
		{
			std::cout << control_set_type << std::endl;
			throw std::string("[XMLDeserializer::createMotionBehaviour] ERROR: Unknown type of rxControlSet.");
		}	
	}

	MotionBehaviour* mb = new MotionBehaviour(dad, son , mb_control_set);

	mb->setMinTimeForInterpolation(deserializeElement<double>(motion_behaviour_xml, "minTime",-1.));
	mb->setMaxVelocityForInterpolation(deserializeElement<double>(motion_behaviour_xml, "maxVelocity",-1.));

	int mb_controller_counter = 0;
	for(TiXmlElement* rxController_xml = control_set_element->FirstChildElement("Controller"); rxController_xml; rxController_xml = rxController_xml->NextSiblingElement())
	{	
		bool mb_goal_controller = deserializeBoolean(rxController_xml, "goalController", true);		
		rxController* mb_controller = XMLDeserializer::createController(rxController_xml, dad, son, robot, dT, mb_goal_controller, mb_controller_counter);
		mb->addController(mb_controller, mb_goal_controller);
		mb_controller_counter++;
	}

    double length = deserializeElement<double>(motion_behaviour_xml, "length", -1.0);
    double prob   = deserializeElement<double>(motion_behaviour_xml, "probability", -1.0);
    mb->setLength(length);
    mb->setProbability(prob);

	return mb;
}

rxController* XMLDeserializer::createController(TiXmlElement *rxController_xml, const Milestone *dad, const Milestone *son, rxSystem *robot, double dT, bool goal_controller, int controller_counter)
{
	// iterate through all attributes (vs. iterate through parameter set)
	// and fill parameter set (check for default values)
	ControllerParameters params;
	params.type = deserializeString(rxController_xml, "type");
	params.ik = deserializeBoolean(rxController_xml, "ik");
	params.kp = deserializeDVector(rxController_xml, "kp");
	params.kv = deserializeDVector(rxController_xml, "kv");
	params.stiffness_b = deserializeDVector(rxController_xml, "stiffness_b");
	params.stiffness_k = deserializeDVector(rxController_xml, "stiffness_k");
	params.invL2sqr = deserializeDVector(rxController_xml, "invL2sqr");
	params.priority = deserializeElement<int>(rxController_xml, "priority", 1);
	params.timeGoal = deserializeElement<double>(rxController_xml, "timeGoal", -1.0);
	params.reuseGoal = deserializeBoolean(rxController_xml, "reuseGoal", false);
	params.typeGoal = deserializeElement<int>(rxController_xml, "typeGoal", 0);
	params.dVectorGoal = deserializeDVector(rxController_xml, "dVectorGoal");
	params.Vector3DGoal = deserializeDVector(rxController_xml, "Vector3DGoal");
	params.RGoal = deserializeRotation(rxController_xml, "RGoal", Rotation());
	params.rGoal = deserializeDVector(rxController_xml, "rGoal");
	params.desired_distance = deserializeElement<double>(rxController_xml, "desiredDistance", 1.);
	params.max_force = deserializeElement<double>(rxController_xml, "maxForce", 1.);
    params.max_vel = deserializeElement<double>(rxController_xml, "maxVel", 1.);
	params.limit_body = deserializeString(rxController_xml, "limitBody", false);
	params.distance_limit = deserializeElement<double>(rxController_xml, "distanceLimit", 0.);
	params.distance_threshold = deserializeElement<double>(rxController_xml, "distanceThreshold", 1.);
	params.deactivation_threshold = deserializeElement<double>(rxController_xml, "deactivationThreshold", 1.);
	params.index = deserializeStdVector<long>(rxController_xml, "index");
	params.safetyThresh = deserializeElement<double>(rxController_xml, "safetyThresh", 1.0);
	params.tc = deserializeStdVector<double>(rxController_xml, "taskConstraints");
	params.alpha = deserializeBody(robot, rxController_xml, "alpha", NULL);
	params.alpha_displacement = deserializeDisplacement(rxController_xml, "alphaDisplacement", Displacement());
	params.alpha_rotation_matrix = deserializeRotation(rxController_xml, "alphaRotation", Rotation());	
	params.beta = deserializeBody(robot, rxController_xml, "beta", NULL);
	if(params.beta != NULL)
	{
		//Use values from xml
		params.beta_displacement = deserializeDisplacement(rxController_xml, "betaDisplacement", Displacement());
		params.beta_rotation_matrix = deserializeRotation(rxController_xml, "betaRotation", Rotation());
	}
	else
	{
		//Set EE as default body
		HTransform transform;
		params.beta = robot->getUCSBody(XMLDeserializer::string2wstring(deserializeString(rxController_xml, "beta", std::string("EE"))), transform);
		params.beta_displacement = transform.r;
		params.beta_rotation_matrix = transform.R;
	}

	//params.beta_displacement = deserializeDisplacement(rxController_xml, "betaDisplacement", Displacement());
	//params.beta_rotation_matrix = deserializeRotation(rxController_xml, "betaRotation", Rotation());
	// viapoints ?

	// call the appropriate factory method which takes the parameter set as input
	rxController* controller = NULL;
	if (params.type == "rxJointController")
	{
		rxJointController* special_controller = new rxJointController(robot, dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		controller = special_controller;
	}
	else if (params.type == "rxJointComplianceController")
	{
		rxJointComplianceController* special_controller = new rxJointComplianceController(robot, dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		special_controller->setStiffness(params.stiffness_b, params.stiffness_k);
		controller = special_controller;
	}
	else if (params.type == "rxJointImpedanceController")
	{
		rxJointImpedanceController* special_controller = new rxJointImpedanceController(robot, dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedJointController")
	{
		rxInterpolatedJointController* special_controller = new rxInterpolatedJointController(robot, dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedJointComplianceController")
	{
		rxInterpolatedJointComplianceController* special_controller = new rxInterpolatedJointComplianceController(robot, dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		special_controller->setStiffness(params.stiffness_b, params.stiffness_k);
		controller = special_controller;
	}
	else if (params.type == "ReInterpolatedJointImpedanceController")
	{
		ReInterpolatedJointImpedanceController* special_controller = new ReInterpolatedJointImpedanceController(robot, dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		special_controller->setImpedance(0.5,3.0,2.0);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedJointImpedanceController")
	{
		rxInterpolatedJointImpedanceController* special_controller = new rxInterpolatedJointImpedanceController(robot, dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		special_controller->setImpedance(0.5,3.0,2.0);
		controller = special_controller;
	}
	else if (params.type == "JointBlackBoardController")
	{
		JointBlackBoardController* special_controller = new JointBlackBoardController(robot, dT);
		special_controller->setBlackBoardVariableName(params.blackboard_variable_name);
		controller = special_controller;
	}
    else if (params.type == "SingularityAvoidanceController")
	{
        HTransform ht;
        rxBody* EE = robot->getUCSBody(_T("EE"), ht);
        controller = new SingularityAvoidanceController(robot, EE, dT, params.maxVel);
    }
    else if (params.type == "rxDisplacementController")
	{
		rxDisplacementController* special_controller = new rxDisplacementController(robot, params.beta, Displacement(params.beta_displacement), params.alpha, Displacement(params.alpha_displacement), dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		controller = special_controller;
	}
    else if (params.type == "rxDisplacementComplianceController")
	{
		rxDisplacementComplianceController* special_controller = new rxDisplacementComplianceController(robot, params.beta, Displacement(params.beta_displacement), params.alpha, Displacement(params.alpha_displacement), dT);
		special_controller->setStiffness(params.stiffness_b, params.stiffness_k);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		controller = special_controller;
	}
    else if (params.type == "rxDisplacementImpedanceController")
	{
		rxDisplacementImpedanceController* special_controller = new rxDisplacementImpedanceController(robot, params.beta, Displacement(params.beta_displacement), params.alpha, Displacement(params.alpha_displacement), dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedDisplacementController")
	{
		rxInterpolatedDisplacementController* special_controller = new rxInterpolatedDisplacementController(robot, params.beta, Displacement(params.beta_displacement), params.alpha, Displacement(params.alpha_displacement), dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedDisplacementComplianceController")
	{
		rxInterpolatedDisplacementComplianceController* special_controller = new rxInterpolatedDisplacementComplianceController(robot, params.beta, Displacement(params.beta_displacement), params.alpha, Displacement(params.alpha_displacement), dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		special_controller->setStiffness(params.stiffness_b, params.stiffness_k);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedDisplacementImpedanceController")
	{
		rxInterpolatedDisplacementImpedanceController* special_controller = new rxInterpolatedDisplacementImpedanceController(robot, params.beta, Displacement(params.beta_displacement), params.alpha, Displacement(params.alpha_displacement), dT);
		special_controller->addPoint(params.dVectorGoal, params.timeGoal, params.reuseGoal, eInterpolatorType_Cubic);
		controller = special_controller;
	}
	else if (params.type == "FeatureAttractorController")
	{
		FeatureAttractorController* special_controller = new FeatureAttractorController(robot, params.beta, Displacement(params.beta_displacement), dT, params.desired_distance, params.max_force);
		special_controller->setImpedance(0.2, 8.0, 20.0);
		controller = special_controller;
	}
	else if (params.type == "OpSpaceSingularityAvoidanceController")
    {
        controller = new OpSpaceSingularityAvoidanceController(robot, params.beta, params.alpha, dT, params.max_vel);
	}
	else if (params.type == "rxOrientationController")
    {
		rxOrientationController* special_controller = new rxOrientationController(robot, params.beta, params.beta_rotation_matrix, params.alpha, params.alpha_rotation_matrix, dT);
		special_controller->addPoint(params.RGoal, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxOrientationComplianceController")
    {
		rxOrientationComplianceController* special_controller = new rxOrientationComplianceController(robot, params.beta, params.beta_rotation_matrix, params.alpha, params.alpha_rotation_matrix, dT);
		special_controller->setStiffness(params.stiffness_b, params.stiffness_k);
		special_controller->addPoint(params.RGoal, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxOrientationImpedanceController")
    {
		rxOrientationImpedanceController* special_controller = new rxOrientationImpedanceController(robot, params.beta, params.beta_rotation_matrix, params.alpha, params.alpha_rotation_matrix, dT);
		special_controller->addPoint(params.RGoal, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedOrientationController")
    {
		rxInterpolatedOrientationController* special_controller = new rxInterpolatedOrientationController(robot, params.beta, params.beta_rotation_matrix, params.alpha, params.alpha_rotation_matrix, dT);
		special_controller->addPoint(params.RGoal, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedOrientationComplianceController")
    {
		rxInterpolatedOrientationComplianceController* special_controller = new rxInterpolatedOrientationComplianceController(robot, params.beta, params.beta_rotation_matrix, params.alpha, params.alpha_rotation_matrix, dT);
		special_controller->setStiffness(params.stiffness_b, params.stiffness_k);
		special_controller->addPoint(params.RGoal, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedOrientationImpedanceController")
    {
		rxInterpolatedOrientationImpedanceController* special_controller = new rxInterpolatedOrientationImpedanceController(robot, params.beta, params.beta_rotation_matrix, params.alpha, params.alpha_rotation_matrix, dT);
		special_controller->addPoint(params.RGoal, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxHTransformController")
    {
		rxHTransformController* special_controller = new rxHTransformController(robot, params.beta, HTransform(params.beta_rotation_matrix, params.beta_displacement), params.alpha, HTransform(params.alpha_rotation_matrix, params.alpha_displacement), dT);
		HTransform goal_HTransform(params.RGoal, params.rGoal);
		special_controller->addPoint(goal_HTransform, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxHTransformComplianceController")
    {
		rxHTransformComplianceController* special_controller = new rxHTransformComplianceController(robot, params.beta, HTransform(params.beta_rotation_matrix, params.beta_displacement), params.alpha, HTransform(params.alpha_rotation_matrix, params.alpha_displacement), dT);
		special_controller->setStiffness(params.stiffness_b, params.stiffness_k);
		HTransform goal_HTransform(params.RGoal, params.rGoal);
		special_controller->addPoint(goal_HTransform, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxHTransformImpedanceController")
    {
		rxHTransformImpedanceController* special_controller = new rxHTransformImpedanceController(robot, params.beta, HTransform(params.beta_rotation_matrix, params.beta_displacement), params.alpha, HTransform(params.alpha_rotation_matrix, params.alpha_displacement), dT);
		HTransform goal_HTransform(params.RGoal, params.rGoal);
		special_controller->addPoint(goal_HTransform, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedHTransformController")
    {
		rxInterpolatedHTransformController* special_controller = new rxInterpolatedHTransformController(robot, params.beta, HTransform(params.beta_rotation_matrix, params.beta_displacement), params.alpha, HTransform(params.alpha_rotation_matrix, params.alpha_displacement), dT);
		HTransform goal_HTransform(params.RGoal, params.rGoal);
		special_controller->addPoint(goal_HTransform, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedHTransformComplianceController")
    {
		rxInterpolatedHTransformComplianceController* special_controller = new rxInterpolatedHTransformComplianceController(robot, params.beta, HTransform(params.beta_rotation_matrix, params.beta_displacement), params.alpha, HTransform(params.alpha_rotation_matrix, params.alpha_displacement), dT);
		special_controller->setStiffness(params.stiffness_b, params.stiffness_k);
		HTransform goal_HTransform(params.RGoal, params.rGoal);
		special_controller->addPoint(goal_HTransform, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxInterpolatedHTransformImpedanceController")
    {
		rxInterpolatedHTransformImpedanceController* special_controller = new rxInterpolatedHTransformImpedanceController(robot, params.beta, HTransform(params.beta_rotation_matrix, params.beta_displacement), params.alpha, HTransform(params.alpha_rotation_matrix, params.alpha_displacement), dT);
		HTransform goal_HTransform(params.RGoal, params.rGoal);
		special_controller->addPoint(goal_HTransform, params.timeGoal, params.reuseGoal);
		controller = special_controller;
	}
	else if (params.type == "rxNullMotionController")
    {
		controller = new rxNullMotionController(robot, dT);
	}
	else if (params.type == "rxNullMotionComplianceController")
    {
		controller = new rxNullMotionComplianceController(robot,  dT);
	}
	else if (params.type == "SubdisplacementController")
    {
		SubdisplacementController* special_controller = new SubdisplacementController(robot, params.beta, params.beta_displacement, params.alpha, params.alpha_displacement, dT, params.index, params.max_force, robot->findBody(string_type(string2wstring(params.limit_body))), params.distance_limit );
		if(params.index.size() == 1)
		{
			special_controller->setTaskConstraints(params.tc[0]);
		}
		else if (params.index.size() == 2)
		{
			special_controller->setTaskConstraints(params.tc[0], params.tc[1]);
		}
		special_controller->setImpedance(0.2, 8.0, 20.0);
		controller = special_controller;
	}
	else if (params.type == "ObstacleAvoidanceController")
    {
		if(CollisionInterface::instance)
		{
			controller = new ObstacleAvoidanceController(robot, params.alpha, params.alpha_displacement, params.distance_threshold, CollisionInterface::instance, dT, params.deactivation_threshold);
		}
	}
	else if (params.type == "JointLimitAvoidanceControllerOnDemand")
    {
		controller = new JointLimitAvoidanceControllerOnDemand(robot, params.index[0], params.safetyThresh, dT); 
	}
	else if (params.type == "InterpolatedSetPointDisplacementController")
	{
		controller = new InterpolatedSetPointDisplacementController(robot, params.beta, params.beta_displacement, params.alpha, params.alpha_displacement, dT);
	}
	else if (params.type == "InterpolatedSetPointOrientationController")
	{
		controller = new InterpolatedSetPointOrientationController(robot, params.beta, params.beta_rotation_matrix, params.alpha, params.alpha_rotation_matrix, dT);
	}
	else if (params.type == "PressureDisplacementController")
	{
		controller = new PressureDisplacementController(robot, params.beta, params.beta_displacement, params.beta_rotation_matrix, params.alpha, params.alpha_displacement, dT);
	}
	else if (params.type == "PressureOrientationController")
	{
		controller = new PressureOrientationController(robot, params.beta, params.beta_rotation_matrix, params.alpha, params.alpha_rotation_matrix, dT);
	}
	else
	{
		// unknown type
		throw std::string("[XMLDeserializer::createController] ERROR: Unknown Controller type.");
	}

	// quasicoord <-- missing
	// functional <-- missing (and more!)

	// we need unique names for all controllers in a single control set, otherwise RLAB will complain
	std::wstringstream wss;
	wss << "controller_" << controller_counter;
	controller->setName(wss.str());

	controller->setPriority(params.priority);
	controller->setIKMode(params.ik);
	controller->setGain(params.kv, params.kp, params.invL2sqr);

	return controller;
}

rxController* XMLDeserializer::createController2(TiXmlElement *rxController_xml, const Milestone *dad, const Milestone *son, 
												rxSystem *robot, double dT, bool goal_controller, int controller_counter)
{
	std::string controller_class_name = deserializeString(rxController_xml, "type");
	ControllerType type_of_controller = XMLDeserializer::controller_map_[controller_class_name];
	bool controller_ik_bool = deserializeBoolean(rxController_xml, "ik");
	std::stringstream kp_vector_ss = std::stringstream(deserializeString(rxController_xml,"kp"));
	std::stringstream kv_vector_ss = std::stringstream(deserializeString(rxController_xml,"kv"));
	std::stringstream invL2sqr_vector_ss = std::stringstream(deserializeString(rxController_xml,"invL2sqr"));
	int priority = deserializeElement<int>(rxController_xml, "priority", 1);

	dVector kp_vector;
	dVector kv_vector;
	dVector invL2sqr_vector;
	kp_vector.all(-1.0);
	double kp_value = -1.0;
	double kv_value = -1.0;
	double invL2sqr_value = -1.0;
	while(kp_vector_ss >> kp_value && kv_vector_ss >> kv_value && invL2sqr_vector_ss >> invL2sqr_value)
	{	
		kp_vector.expand(1,kp_value);		
		kv_vector.expand(1,kv_value);		
		invL2sqr_vector.expand(1,invL2sqr_value);
	}

	std::vector<ViaPointBase*> via_points_ptr;

	// Create the Controller
	rxController* controller = NULL;

	switch(type_of_controller.first)
	{
	case rxController::eControlType_Joint:
		controller = XMLDeserializer::createJointController(type_of_controller.second, dT, via_points_ptr, rxController_xml, robot);
		break;
	case rxController::eControlType_Displacement:
		controller = XMLDeserializer::createDisplacementController(type_of_controller.second, dT, via_points_ptr, rxController_xml, robot);
		break;
	case rxController::eControlType_Orientation:
		controller = XMLDeserializer::createOrientationController(type_of_controller.second, dT, via_points_ptr, rxController_xml, robot);
		break;
	case rxController::eControlType_HTransform:
		controller = XMLDeserializer::createHTransformController(type_of_controller.second, dT, via_points_ptr, rxController_xml, robot);
		break;
	case rxController::eControlType_QuasiCoord:
		controller = XMLDeserializer::createQuasiCoordController(type_of_controller.second, dT, robot);
		break;
	case rxController::eControlType_NullMotion:
		controller = XMLDeserializer::createNullMotionController(type_of_controller.second, dT, robot);
		break;
	case rxController::eControlType_Functional:
		{
		controller = XMLDeserializer::createFunctionalController(type_of_controller.second, dT, rxController_xml, robot);
		break;
		}
	default:
		throw std::string("[XMLDeserializer::createController] ERROR: Unexpected Controller group.");
		break;
	}

	// we need unique names for all controllers in a single control set, otherwise RLAB will complain
	std::wstringstream wss;
	wss << "controller_" << controller_counter;
	controller->setName(wss.str());

	if(!goal_controller && !(type_of_controller.second & OBSTACLE_AVOIDANCE) &&!(type_of_controller.second & SUBDISPLACEMENT) &&!(type_of_controller.second & SINGULARITY_AVOIDANCE) &&!(type_of_controller.second & JOINT_LIMIT_AVOIDANCE))
	{

		double time_goal = deserializeElement<double>(rxController_xml, "timeGoal");
		bool reuse_goal = deserializeBoolean(rxController_xml, "reuseGoal");
		int type_goal = deserializeElement<int>(rxController_xml, "typeGoal");

		switch(controller->type()) {
	case rxController::eControlType_Joint:
		{
			std::stringstream goal_ss = std::stringstream(deserializeString(rxController_xml,"dVectorGoal"));
			dVector goal_vector;
			double goal_value = -1.0;
			while(goal_ss >> goal_value)
			{	
				goal_vector.expand(1,goal_value);		
			}
			dynamic_cast<rxJointController*>(controller)->addPoint(goal_vector, time_goal, reuse_goal, eInterpolatorType_Cubic);
			break;
		}
	case rxController::eControlType_Displacement:
		{
			std::stringstream goal_ss = std::stringstream(deserializeString(rxController_xml,"Vector3DGoal"));
			Vector3D goal_vector;
			double goal_value = -1.0;
			while(goal_ss >> goal_value)
			{	
				goal_vector.expand(1,goal_value);		
			}
			dynamic_cast<rxDisplacementController*>(controller)->addPoint(goal_vector, time_goal, reuse_goal, eInterpolatorType_Cubic);
			break;
		}
	case rxController::eControlType_Orientation:
		{
			std::stringstream goal_ss = std::stringstream(deserializeString(rxController_xml, "RGoal"));
			double goal_value = -1.0;
			std::vector<double> goal_values;
			for(int i=0; i<9; i++)
			{
				goal_ss >> goal_value;
				goal_values.push_back(goal_value);
			}
			Rotation goal_Rotation(goal_values[0], goal_values[1],goal_values[2],goal_values[3],
				goal_values[4],goal_values[5],goal_values[6],goal_values[7],goal_values[8]);
			dynamic_cast<rxOrientationController*>(controller)->addPoint(goal_Rotation, time_goal, reuse_goal);
			break;
		}
	case rxController::eControlType_HTransform:
		{
			std::stringstream goal_ss = std::stringstream(deserializeString(rxController_xml, "RGoal"));
			double goal_value = -1.0;
			std::vector<double> goal_values;
			for(int i=0; i<9; i++)
			{
				goal_ss >> goal_value;
				goal_values.push_back(goal_value);
			}
			Rotation goal_Rotation(goal_values[0], goal_values[1],goal_values[2],goal_values[3],
				goal_values[4],goal_values[5],goal_values[6],goal_values[7],goal_values[8]);

			std::stringstream goal_ss2 = std::stringstream(deserializeString(rxController_xml, "rGoal"));
			goal_values.clear();
			for(int i=0; i<9; i++)
			{
				goal_ss2 >> goal_value;
				goal_values.push_back(goal_value);
			}
			Displacement goal_Displacement(goal_values[0], goal_values[1],goal_values[2]);		

			HTransform goal_HTransform(goal_Rotation, goal_Displacement);
			dynamic_cast<rxHTransformController*>(controller)->addPoint(goal_HTransform, time_goal, reuse_goal);
			break;
		}
		}
	}

	controller->setPriority(priority);
	controller->setIKMode(controller_ik_bool);
	controller->setGain(kv_vector, kp_vector, invL2sqr_vector);	
	return controller;
}

rxController* XMLDeserializer::createJointController(int joint_subtype, double controller_duration, std::vector<ViaPointBase*> via_points_ptr, 
													 TiXmlElement* rxController_xml, rxSystem* robot)
{
	rxController* controller = NULL;
	switch(joint_subtype)
	{
	case NONE:
		controller = new rxJointController(robot, controller_duration);
		break;
	case WITH_COMPLIANCE:
		controller = new rxJointComplianceController(robot, controller_duration);	
		break;
	case WITH_IMPEDANCE:
		controller = new rxJointImpedanceController(robot, controller_duration);
		break;
	case WITH_INTERPOLATION:
		controller = new rxInterpolatedJointController(robot, controller_duration);
		break;
	case WITH_INTERPOLATION | WITH_COMPLIANCE:
		controller = new rxInterpolatedJointComplianceController(robot, controller_duration);
		break;
	case WITH_INTERPOLATION | WITH_IMPEDANCE:
		controller = new ReInterpolatedJointImpedanceController(robot, controller_duration);	dynamic_cast<ReInterpolatedJointImpedanceController*>(controller)->setImpedance(0.1,1.0,2.0);
		break;
	case BLACKBOARD_ACCESS:
		controller = new JointBlackBoardController(robot, controller_duration);
		break;
    case SINGULARITY_AVOIDANCE:
        {
            double max_vel = deserializeElement<double>(rxController_xml,"maxVel");
            HTransform ht;
            rxBody* EE = robot->getUCSBody(_T("EE"),ht);
            controller = new SingularityAvoidanceController(robot, EE, controller_duration, max_vel);
        }
        break;
    
	default:
		throw std::string("[XMLDeserializer::createJointController] ERROR: Unexpected Controller subgroup.");
		break;
	}
	for(std::vector<ViaPointBase*>::iterator vp_it = via_points_ptr.begin(); vp_it != via_points_ptr.end(); vp_it++)
	{
		ViaPointdVector* vpdv_ptr = (ViaPointdVector*)((*vp_it));
		dynamic_cast<rxJointController*>(controller)->addPoint(vpdv_ptr->point_, vpdv_ptr->time_, vpdv_ptr->reuse_, (eInterpolatorType)(vpdv_ptr->type_));
	}

	if(joint_subtype & WITH_COMPLIANCE)
	{
		std::stringstream stiffness_b_ss = std::stringstream(deserializeString(rxController_xml,"stiffness_b"));
		std::stringstream stiffness_k_ss = std::stringstream(deserializeString(rxController_xml,"stiffness_k"));
		double stiffness_b_value = -1.0;
		double stiffness_k_value = -1.0;
		dVector stiffness_b_displacement;
		dVector stiffness_k_displacement;
		while ((stiffness_b_ss >> stiffness_b_value))
		{
			stiffness_b_displacement.expand(1,stiffness_b_value);
		}
		while ((stiffness_k_ss >> stiffness_k_value))
		{
			stiffness_k_displacement.expand(1,stiffness_k_value);
		}
		dynamic_cast<rxJointComplianceController*>(controller)->setStiffness(stiffness_b_displacement, stiffness_k_displacement);
	}

	if (joint_subtype & BLACKBOARD_ACCESS)
	{
		// parse the name of the blackboard variable to be accessed
		// if not given --- use default
		JointBlackBoardController* bb_controller = dynamic_cast<JointBlackBoardController*>(controller);
		bb_controller->setBlackBoardVariableName(bb_controller->getBlackBoardVariableName());
	}

	return controller;
}

rxController* XMLDeserializer::createDisplacementController(int displacement_subtype, double controller_duration, 
															std::vector<ViaPointBase*> via_points_ptr, TiXmlElement* rxController_xml, rxSystem* robot)
{
	rxBody*   alpha = NULL;
	rxBody*   beta = NULL;
	rxController* controller = NULL;
	dVector alpha_displacement;
	std::string alpha_str = deserializeString(rxController_xml,"alpha", false);
	alpha = robot->findBody(string2wstring(alpha_str));
	if(alpha)
	{
		std::stringstream alpha_ss = std::stringstream(deserializeString(rxController_xml,"alphaDisplacement"));
		double alpha_value = -1.0;
		while ((alpha_ss >> alpha_value))
		{
			alpha_displacement.expand(1,alpha_value);
		}
	}else{
		alpha_displacement = Displacement();
	}

	dVector beta_displacement;
	std::string beta_str = deserializeString(rxController_xml,"beta", false);
	beta = robot->findBody(string2wstring(beta_str));
	if(beta)
	{
		std::stringstream beta_ss = std::stringstream(deserializeString(rxController_xml,"betaDisplacement"));
		double beta_value = -1.0;	
		while((beta_ss >> beta_value))
		{
			beta_displacement.expand(1,beta_value);
		}
	}else{
		beta_displacement = Displacement();
	}

	switch(displacement_subtype)
	{
	case NONE:
		controller = new rxDisplacementController(robot, beta, Displacement(beta_displacement), alpha, Displacement(alpha_displacement), controller_duration);
		break;
	case WITH_COMPLIANCE:
		{
			controller = new rxDisplacementComplianceController(robot, beta, Displacement(beta_displacement), alpha, Displacement(alpha_displacement), controller_duration );
			break;
		}
	case WITH_IMPEDANCE:
		controller = new rxDisplacementImpedanceController(robot, beta, Displacement(beta_displacement), alpha, Displacement(alpha_displacement), controller_duration );
		break;
	case WITH_INTERPOLATION:
		controller = new rxInterpolatedDisplacementController(robot, beta, Displacement(beta_displacement), alpha, Displacement(alpha_displacement), controller_duration );
		break;
	case WITH_INTERPOLATION | WITH_COMPLIANCE:
		{
			controller = new rxInterpolatedDisplacementComplianceController(robot, beta, Displacement(beta_displacement), alpha, Displacement(alpha_displacement), controller_duration );
			break;
		}
	case WITH_INTERPOLATION | WITH_IMPEDANCE:
		controller = new rxInterpolatedDisplacementImpedanceController(robot, beta, Displacement(beta_displacement), alpha, Displacement(alpha_displacement), controller_duration );
		break;
	case WITH_IMPEDANCE | ATTRACTOR:
		{
			double desired_distance = deserializeElement<double>(rxController_xml,"desiredDistance", 1.);
			double max_force = deserializeElement<double>(rxController_xml,"maxForce", 1.);
			controller = new FeatureAttractorController(robot, beta, Displacement(beta_displacement), controller_duration, desired_distance, max_force );
			dynamic_cast<FeatureAttractorController*>(controller)->setImpedance(0.2,8.0,20.0);
			break;
		}
    case SINGULARITY_AVOIDANCE:
        {
            double max_vel = deserializeElement<double>(rxController_xml,"maxVel", 1.);
            controller = new OpSpaceSingularityAvoidanceController(robot, beta, alpha, controller_duration, max_vel );
			break;
        }
	default:
		throw std::string("[XMLDeserializer::createDisplacementController] ERROR: Unexpected Controller subgroup.");
		break;
	}
	for(std::vector<ViaPointBase*>::iterator vp_it = via_points_ptr.begin(); vp_it != via_points_ptr.end(); vp_it++)
	{
		ViaPointVector3D* vpdv_ptr = (ViaPointVector3D*)((*vp_it));
		dynamic_cast<rxDisplacementController*>(controller)->addPoint(vpdv_ptr->point_, vpdv_ptr->time_, vpdv_ptr->reuse_, (eInterpolatorType)(vpdv_ptr->type_));
	}

	if(displacement_subtype & WITH_COMPLIANCE)
	{
		std::stringstream stiffness_b_ss = std::stringstream(deserializeString(rxController_xml,"stiffness_b"));
		std::stringstream stiffness_k_ss = std::stringstream(deserializeString(rxController_xml,"stiffness_k"));
		double stiffness_b_value = -1.0;
		double stiffness_k_value = -1.0;
		dVector stiffness_b_displacement;
		dVector stiffness_k_displacement;
		while ((stiffness_b_ss >> stiffness_b_value))
		{
			stiffness_b_displacement.expand(1,stiffness_b_value);
		}
		while ((stiffness_k_ss >> stiffness_k_value))
		{
			stiffness_k_displacement.expand(1,stiffness_k_value);
		}
		dynamic_cast<rxDisplacementComplianceController*>(controller)->setStiffness(stiffness_b_displacement, stiffness_k_displacement);
	}
	return controller;
}

rxController* XMLDeserializer::createOrientationController(int orientation_subtype, double controller_duration, std::vector<ViaPointBase*> via_points_ptr,
														   TiXmlElement* rxController_xml, rxSystem* robot)
{
	rxBody*   alpha = NULL;
	rxBody*   beta = NULL;
	rxController* controller = NULL;
	Rotation alpha_rotation_matrix;
	Rotation beta_rotation_matrix;
	std::string alpha_str = deserializeString(rxController_xml,"alpha",false);
	alpha = robot->findBody(string2wstring(alpha_str));
	if(alpha)
	{
		alpha_rotation_matrix = string2rotation(deserializeString(rxController_xml,"alphaRotation"), Rotation());
	}
	else
	{
		alpha_rotation_matrix = Rotation();
	}
	std::string beta_str = deserializeString(rxController_xml,"beta",false);
	beta = robot->findBody(string2wstring(beta_str));
	if(beta)
	{
		beta_rotation_matrix = string2rotation(deserializeString(rxController_xml,"betaRotation"), Rotation());
	}
	else
	{
		beta_rotation_matrix = Rotation();
	}

	switch(orientation_subtype)
	{
	case NONE:
		controller = new rxOrientationController(robot, beta, beta_rotation_matrix, alpha, alpha_rotation_matrix, controller_duration );
		break;
	case WITH_COMPLIANCE:
		{
			controller = new rxOrientationComplianceController(robot, beta, beta_rotation_matrix, alpha,alpha_rotation_matrix, controller_duration );
			break;
		}
	case WITH_IMPEDANCE:
		controller = new rxOrientationImpedanceController(robot, beta, beta_rotation_matrix, alpha,alpha_rotation_matrix, controller_duration );
		break;
	case WITH_INTERPOLATION:
		controller = new rxInterpolatedOrientationController(robot, beta, beta_rotation_matrix, alpha,alpha_rotation_matrix, controller_duration );
		break;
	case WITH_INTERPOLATION | WITH_COMPLIANCE:
		{
			controller = new rxInterpolatedOrientationComplianceController(robot, beta, beta_rotation_matrix, alpha,alpha_rotation_matrix, controller_duration );
			break;
		}
	case WITH_INTERPOLATION | WITH_IMPEDANCE:
		controller = new rxInterpolatedOrientationImpedanceController(robot, beta, beta_rotation_matrix, alpha,alpha_rotation_matrix, controller_duration );
		break;
	default:
		throw std::string("[XMLDeserializer::createOrientationController] ERROR:  Unexpected Controller subgroup.");
		break;
	}
	for(std::vector<ViaPointBase*>::iterator vp_it = via_points_ptr.begin(); vp_it != via_points_ptr.end(); vp_it++)
	{
		ViaPointRotation* vpdv_ptr = (ViaPointRotation*)((*vp_it));
		dynamic_cast<rxOrientationController*>(controller)->addPoint(vpdv_ptr->point_, vpdv_ptr->time_, vpdv_ptr->reuse_, (eInterpolatorType)(vpdv_ptr->type_));
	}
	if(orientation_subtype & WITH_COMPLIANCE)
	{
		std::stringstream stiffness_b_ss(deserializeString(rxController_xml,"stiffness_b"));
		std::stringstream stiffness_k_ss(deserializeString(rxController_xml,"stiffness_k"));
		double stiffness_b_value = -1.0;
		double stiffness_k_value = -1.0;
		dVector stiffness_b_displacement;
		dVector stiffness_k_displacement;
		while ((stiffness_b_ss >> stiffness_b_value))
		{
			stiffness_b_displacement.expand(1,stiffness_b_value);
		}
		while ((stiffness_k_ss >> stiffness_k_value))
		{
			stiffness_k_displacement.expand(1,stiffness_k_value);
		}
		dynamic_cast<rxOrientationComplianceController*>(controller)->setStiffness(stiffness_b_displacement, stiffness_k_displacement);
	}
	return controller;
}

rxController* XMLDeserializer::createHTransformController(int htransform_subtype, double controller_duration, std::vector<ViaPointBase*> via_points_ptr,
														  TiXmlElement* rxController_xml, rxSystem* robot)
{
    rxBody*   alpha = NULL;
	rxBody*   beta = NULL;
	rxController* controller = NULL;
	Rotation alpha_rotation_matrix;
    Displacement alpha_displacement;
	Rotation beta_rotation_matrix;
    Displacement beta_displacement;
	std::string alpha_str = deserializeString(rxController_xml,"alpha",false);
	alpha = robot->findBody(string2wstring(alpha_str));
	if(alpha)
	{
		alpha_rotation_matrix = string2rotation(deserializeString(rxController_xml,"alphaRotation"), Rotation());
        
        std::stringstream alpha_ss = std::stringstream(deserializeString(rxController_xml,"alphaDisplacement"));
		double alpha_value = -1.0;
		while ((alpha_ss >> alpha_value))
		{
			alpha_displacement.expand(1,alpha_value);
		}
	}
	else
	{
		alpha_rotation_matrix = Rotation();
        alpha_displacement = Displacement();
	}

	std::string beta_str = deserializeString(rxController_xml,"beta",false);
	beta = robot->findBody(string2wstring(beta_str));
	if(beta)
	{
		beta_rotation_matrix = string2rotation(deserializeString(rxController_xml,"betaRotation"), Rotation());
        
        std::stringstream beta_ss = std::stringstream(deserializeString(rxController_xml,"betaDisplacement"));
		double beta_value = -1.0;
		while ((beta_ss >> beta_value))
		{
			beta_displacement.expand(1,beta_value);
		}
	}
	else
	{
		beta_rotation_matrix = Rotation();
        beta_displacement = Displacement();
	}

	switch(htransform_subtype)
	{
	case NONE:
		controller = new rxHTransformController(robot, beta, HTransform(beta_rotation_matrix, beta_displacement), alpha, HTransform(alpha_rotation_matrix, alpha_displacement), controller_duration );
		break;
	case WITH_COMPLIANCE:
		controller = new rxHTransformComplianceController(robot, beta, HTransform(beta_rotation_matrix, beta_displacement), alpha, HTransform(alpha_rotation_matrix, alpha_displacement), controller_duration );
		break;
	case WITH_IMPEDANCE:
		controller = new rxHTransformImpedanceController(robot, beta, HTransform(beta_rotation_matrix, beta_displacement), alpha, HTransform(alpha_rotation_matrix, alpha_displacement), controller_duration );
		break;
	case WITH_INTERPOLATION:
		controller = new rxInterpolatedHTransformController(robot, beta, HTransform(beta_rotation_matrix, beta_displacement), alpha, HTransform(alpha_rotation_matrix, alpha_displacement), controller_duration );
		break;
	case WITH_INTERPOLATION | WITH_COMPLIANCE:
		controller = new rxInterpolatedHTransformComplianceController(robot, beta, HTransform(beta_rotation_matrix, beta_displacement), alpha, HTransform(alpha_rotation_matrix, alpha_displacement), controller_duration );
		break;
	case WITH_INTERPOLATION | WITH_IMPEDANCE:
		controller = new rxInterpolatedHTransformImpedanceController(robot, beta, HTransform(beta_rotation_matrix, beta_displacement), alpha, HTransform(alpha_rotation_matrix, alpha_displacement), controller_duration );
		break;
	default:
		throw std::string("[XMLDeserializer::createHTransformController] ERROR: Unexpected Controller subgroup.");
		break;
	}
	for(std::vector<ViaPointBase*>::iterator vp_it = via_points_ptr.begin(); vp_it != via_points_ptr.end(); vp_it++)
	{
		ViaPointHTransform* vpdv_ptr = (ViaPointHTransform*)((*vp_it));
		dynamic_cast<rxHTransformController*>(controller)->addPoint(vpdv_ptr->point_, vpdv_ptr->time_, vpdv_ptr->reuse_, (eInterpolatorType)(vpdv_ptr->type_));
	}

	if(htransform_subtype & WITH_COMPLIANCE)
	{
		std::stringstream stiffness_b_ss = std::stringstream(deserializeString(rxController_xml,"stiffness_b"));
		std::stringstream stiffness_k_ss = std::stringstream(deserializeString(rxController_xml,"stiffness_k"));
		double stiffness_b_value = -1.0;
		double stiffness_k_value = -1.0;
		dVector stiffness_b_displacement;
		dVector stiffness_k_displacement;
		while ((stiffness_b_ss >> stiffness_b_value))
		{
			stiffness_b_displacement.expand(1,stiffness_b_value);
		}
		while ((stiffness_k_ss >> stiffness_k_value))
		{
			stiffness_k_displacement.expand(1,stiffness_k_value);
		}
		dynamic_cast<rxHTransformComplianceController*>(controller)->setStiffness(stiffness_b_displacement, stiffness_k_displacement);
	}
	return controller;
}

rxController* XMLDeserializer::createQuasiCoordController(int quasi_coord_subtype, double controller_duration, rxSystem* robot)
{
	rxController* controller = NULL;
	switch(quasi_coord_subtype)
	{
	case NONE:
		//	controller = new rxQuasiCoordController(robot, , controller_duration);
		break;
	case WITH_COMPLIANCE:
		//	controller = new rxQuasiCoordComplianceController(robot, ,controller_duration);
		break;
	default:
		throw std::string("[XMLDeserializer::createQuasiCoordController] ERROR: Unexpected Controller subgroup.");
		break;
	}
	return controller;
}

rxController* XMLDeserializer::createNullMotionController(int null_motion_subtype, double controller_duration, rxSystem * robot)
{
	rxController* controller = NULL;
	switch(null_motion_subtype)
	{
	case NONE:
		controller = new rxNullMotionController(robot, controller_duration);
		break;
	case WITH_COMPLIANCE:
		controller = new rxNullMotionComplianceController(robot,  controller_duration);
		break;
	case WITH_GRADIENT:
		//	controller = new rxGradientNullMotionController(robot, controller_duration);
		break;
	default:
		throw std::string("[XMLDeserializer::createNullMotionController] ERROR: Unexpected Controller subgroup.");
		break;
	}
	return controller;
}

rxController* XMLDeserializer::createFunctionalController(int functional_subtype, double controller_duration, TiXmlElement* rxController_xml, rxSystem* robot)
{
	rxController* controller = NULL;
	switch(functional_subtype)
	{
	case WITH_IMPEDANCE | SUBDISPLACEMENT:
		{
			rxBody*   alpha = NULL;
			rxBody*   beta = NULL;
			dVector alpha_displacement;
			std::string alpha_str = deserializeString(rxController_xml,"alpha", false);

			alpha = robot->findBody(string2wstring(alpha_str));

			if(alpha)
			{
				std::stringstream alpha_ss = std::stringstream(deserializeString(rxController_xml,"alphaDisplacement"));
				double alpha_value = -1.0;
				while ((alpha_ss >> alpha_value))
				{
					alpha_displacement.expand(1,alpha_value);
				}
			}else{
				alpha_displacement = dVector(3,0.);
			}

			dVector beta_displacement;
			std::string beta_str = deserializeString(rxController_xml,"beta", false);
			beta = robot->findBody(string2wstring(beta_str));
			if(beta)
			{
				std::stringstream beta_ss = std::stringstream(deserializeString(rxController_xml,"betaDisplacement"));
				double beta_value = -1.0;	
				while((beta_ss >> beta_value))
				{
					beta_displacement.expand(1,beta_value);
				}
			}else{
				beta_displacement = dVector(3,0.);
			}
			std::stringstream index_ss = std::stringstream(deserializeString(rxController_xml,"index"));
			std::vector<long> index;
			long index_value;
			while((index_ss >> index_value))
			{
				index.push_back(index_value);
			}

			std::stringstream tc_ss = std::stringstream(deserializeString(rxController_xml,"taskConstraints"));
			std::vector<double> tc;
			double tc_value;
			while((tc_ss >> tc_value))
			{
				tc.push_back(tc_value);
			}

			double max_force = deserializeElement<double>(rxController_xml,"maxForce", 1.);
			std::string limit_body = deserializeString(rxController_xml,"limitBody");
			double distance_limit = deserializeElement<double>(rxController_xml,"distanceLimit", 0.);
			controller = new SubdisplacementController(robot,beta, beta_displacement, alpha, alpha_displacement,
				controller_duration, index, max_force, robot->findBody(string_type(string2wstring(limit_body))), distance_limit );
			if(index.size() == 1)
			{
				dynamic_cast<SubdisplacementController*>(controller)->setTaskConstraints(tc[0]);
			}else if (index.size() ==2)
			{
				dynamic_cast<SubdisplacementController*>(controller)->setTaskConstraints(tc[0], tc[1]);
			}
			dynamic_cast<SubdisplacementController*>(controller)->setImpedance(0.2,8.0,20.0);
		}
		break;
	case OBSTACLE_AVOIDANCE:
		{
			rxBody*   alpha = NULL;
			std::string alpha_str = deserializeString(rxController_xml,"alpha");
			alpha = robot->findBody(string2wstring(alpha_str));
			std::stringstream alpha_disp_ss = std::stringstream(deserializeString(rxController_xml,"alphaDisplacement"));
			dVector alpha_displacement;
			double alpha_value = -1.0;
			while((alpha_disp_ss >> alpha_value))
			{
				alpha_displacement.expand(1,alpha_value);
			}

			double distance_threshold = deserializeElement<double>(rxController_xml,"distanceThreshold", 1.);
			//cout << "distance_threshold " << distance_threshold << endl;
			double deactivation_threshold = deserializeElement<double>(rxController_xml,"deactivationThreshold", 0.);
			//cout << "deactivation_threshold " << deactivation_threshold << endl;
			if(CollisionInterface::instance)
			{
				controller = new ObstacleAvoidanceController(robot, alpha, alpha_displacement, distance_threshold, 
					CollisionInterface::instance, controller_duration, deactivation_threshold);
			}else{
				throw std::string("[XMLDeserializer::createFunctionalController] ERROR Obstacle Avoidance needs Collision interface.");
			}
		}
		break;
   case JOINT_LIMIT_AVOIDANCE:
		{
			long index = (long)deserializeElement<int>(rxController_xml,"index");
			double safetyThresh = deserializeElement<double>(rxController_xml,"safetyThresh", 1.0);
            controller = new JointLimitAvoidanceControllerOnDemand(robot, index, safetyThresh, controller_duration); 
		}
		break;

	default:
		throw std::string("[XMLDeserializer::createFunctionalController] ERROR: Unexpected Controller subgroup.");
		break;
	}
	return controller;
}

std::map<std::string, ControllerType> XMLDeserializer::createControllerMapping()
{
	std::map<std::string, ControllerType> mapping;
	mapping["rxJointController"]							= ControllerType(rxController::eControlType_Joint, NONE);
	mapping["rxJointComplianceController"]					= ControllerType(rxController::eControlType_Joint, WITH_COMPLIANCE);
	mapping["rxJointImpedanceController"]					= ControllerType(rxController::eControlType_Joint, WITH_IMPEDANCE);
	mapping["rxInterpolatedJointController"]				= ControllerType(rxController::eControlType_Joint, WITH_INTERPOLATION);
	mapping["rxInterpolatedJointComplianceController"]		= ControllerType(rxController::eControlType_Joint, WITH_INTERPOLATION | WITH_COMPLIANCE);
	mapping["rxInterpolatedJointImpedanceController"]		= ControllerType(rxController::eControlType_Joint, WITH_INTERPOLATION | WITH_IMPEDANCE);

	mapping["rxFunctionController"]							= ControllerType(rxController::eControlType_Functional, NONE);
	mapping["rxFunctionComplianceController"]				= ControllerType(rxController::eControlType_Functional, WITH_COMPLIANCE);
	mapping["rxFunctionImpedanceController"]				= ControllerType(rxController::eControlType_Functional, WITH_IMPEDANCE);
	mapping["rxInterpolatedFunctionController"]				= ControllerType(rxController::eControlType_Functional, WITH_INTERPOLATION);
	mapping["rxInterpolatedFunctionComplianceController"]	= ControllerType(rxController::eControlType_Functional, WITH_INTERPOLATION | WITH_COMPLIANCE);
	mapping["rxInterpolatedFunctionImpedanceController"]	= ControllerType(rxController::eControlType_Functional, WITH_INTERPOLATION | WITH_IMPEDANCE);

	mapping["rxDisplacementController"]						= ControllerType(rxController::eControlType_Displacement, NONE);
	mapping["rxDisplacementFunctionController"]				= ControllerType(rxController::eControlType_Displacement, WITH_FUNCTION);
	mapping["rxDisplacementFunctionComplianceController"]	= ControllerType(rxController::eControlType_Displacement, WITH_FUNCTION | WITH_COMPLIANCE);
	mapping["rxDisplacementFunctionImpedanceController"]	= ControllerType(rxController::eControlType_Displacement, WITH_FUNCTION | WITH_IMPEDANCE);
	mapping["rxDisplacementComplianceController"]			= ControllerType(rxController::eControlType_Displacement, WITH_COMPLIANCE);
	mapping["rxDisplacementImpedanceController"]			= ControllerType(rxController::eControlType_Displacement, WITH_IMPEDANCE);
	mapping["rxInterpolatedDisplacementController"]			= ControllerType(rxController::eControlType_Displacement, WITH_INTERPOLATION);
	mapping["rxInterpolatedDisplacementComplianceController"] = ControllerType(rxController::eControlType_Displacement, WITH_INTERPOLATION | WITH_COMPLIANCE);
	mapping["rxInterpolatedDisplacementImpedanceController"] = ControllerType(rxController::eControlType_Displacement, WITH_INTERPOLATION | WITH_IMPEDANCE);

	mapping["rxOrientationController"]						= ControllerType(rxController::eControlType_Orientation, NONE);
	mapping["rxOrientationFunctionController"]				= ControllerType(rxController::eControlType_Orientation, WITH_FUNCTION);
	mapping["rxOrientationFunctionComplianceController"]	= ControllerType(rxController::eControlType_Orientation, WITH_FUNCTION | WITH_COMPLIANCE);
	mapping["rxOrientationFunctionImpedanceController"]		= ControllerType(rxController::eControlType_Orientation, WITH_FUNCTION | WITH_IMPEDANCE);
	mapping["rxOrientationComplianceController"]			= ControllerType(rxController::eControlType_Orientation, WITH_COMPLIANCE);
	mapping["rxOrientationImpedanceController"]				= ControllerType(rxController::eControlType_Orientation, WITH_IMPEDANCE);
	mapping["rxInterpolatedOrientationController"]			= ControllerType(rxController::eControlType_Orientation, WITH_INTERPOLATION);
	mapping["rxInterpolatedOrientationComplianceController"] = ControllerType(rxController::eControlType_Orientation, WITH_INTERPOLATION | WITH_COMPLIANCE);
	mapping["rxInterpolatedOrientationImpedanceController"] = ControllerType(rxController::eControlType_Orientation, WITH_INTERPOLATION | WITH_IMPEDANCE);

	mapping["rxHTransformController"]						= ControllerType(rxController::eControlType_HTransform, NONE);
	mapping["rxHTransformFunctionController"]				= ControllerType(rxController::eControlType_HTransform, WITH_FUNCTION);
	mapping["rxHTransformFunctionComplianceController"]		= ControllerType(rxController::eControlType_HTransform, WITH_FUNCTION | WITH_COMPLIANCE);
	mapping["rxHTransformFunctionImpedanceController"]		= ControllerType(rxController::eControlType_HTransform, WITH_FUNCTION | WITH_IMPEDANCE);
	mapping["rxHTransformComplianceController"]				= ControllerType(rxController::eControlType_HTransform, WITH_COMPLIANCE);
	mapping["rxHTransformImpedanceController"]				= ControllerType(rxController::eControlType_HTransform, WITH_IMPEDANCE);
	mapping["rxInterpolatedHTransformController"]			= ControllerType(rxController::eControlType_HTransform, WITH_INTERPOLATION);
	mapping["rxInterpolatedHTransformComplianceController"] = ControllerType(rxController::eControlType_HTransform, WITH_INTERPOLATION | WITH_COMPLIANCE);
	mapping["rxInterpolatedHTransformImpedanceController"]	= ControllerType(rxController::eControlType_HTransform, WITH_INTERPOLATION | WITH_IMPEDANCE);

	mapping["rxQuasiCoordController"]						= ControllerType(rxController::eControlType_QuasiCoord, NONE);
	mapping["rxQuasiCoordComplianceController"]				= ControllerType(rxController::eControlType_QuasiCoord, WITH_COMPLIANCE);

	mapping["rxNullMotionController"]						= ControllerType(rxController::eControlType_NullMotion, NONE);
	mapping["rxNullMotionComplianceController"]				= ControllerType(rxController::eControlType_NullMotion, WITH_COMPLIANCE);
	mapping["rxGradientNullMotionController"]				= ControllerType(rxController::eControlType_NullMotion, WITH_GRADIENT);

	mapping["FeatureAttractorController"]					= ControllerType(rxController::eControlType_Displacement, WITH_IMPEDANCE | ATTRACTOR);
	mapping["SubdisplacementController"]					= ControllerType(rxController::eControlType_Functional, WITH_IMPEDANCE | SUBDISPLACEMENT);
	mapping["ObstacleAvoidanceController"]					= ControllerType(rxController::eControlType_Functional, OBSTACLE_AVOIDANCE);
	mapping["JointBlackBoardController"]					= ControllerType(rxController::eControlType_Joint, BLACKBOARD_ACCESS);

    mapping["SingularityAvoidanceController"]				= ControllerType(rxController::eControlType_Joint, SINGULARITY_AVOIDANCE);
    mapping["OpSpaceSingularityAvoidanceController"]    	= ControllerType(rxController::eControlType_Displacement, SINGULARITY_AVOIDANCE);
    mapping["JointLimitAvoidanceControllerOnDemand"]		= ControllerType(rxController::eControlType_Functional, JOINT_LIMIT_AVOIDANCE);

	mapping["InterpolatedSetPointDisplacementController"]	= ControllerType(rxController::eControlType_Displacement, WITH_INTERPOLATION);
	mapping["InterpolatedSetPointOrientationController"]	= ControllerType(rxController::eControlType_Orientation, WITH_INTERPOLATION);

	return mapping;
}