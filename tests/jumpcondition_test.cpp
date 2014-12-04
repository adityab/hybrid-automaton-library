#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <string>

#include "hybrid_automaton/JumpCondition.h"
#include "hybrid_automaton/DescriptionTreeNode.h"
#include "hybrid_automaton/JointConfigurationSensor.h"
#include "tests/MockDescriptionTree.h"
#include "tests/MockDescriptionTreeNode.h"

using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using ::testing::AtLeast;
using ::testing::_;

using namespace ::ha;

namespace JumpConditionSerialization1 {
	class MockSystem : public ha::System {
	public:
		MOCK_CONST_METHOD0(getDof, int () );
		MOCK_CONST_METHOD0(getConfiguration, ::Eigen::MatrixXd () );
		MOCK_CONST_METHOD0(getForceTorqueMeasurement, ::Eigen::MatrixXd () );
		MOCK_CONST_METHOD0(getCurrentTime, ::Eigen::MatrixXd () );
		MOCK_CONST_METHOD1(getFramePose, ::Eigen::MatrixXd (const std::string& frame_id) );
	};
}

TEST(JumpCondition, Serialization) {
	using namespace ha;
	using namespace std;

	//-------
	// Serialized and deserialized sensor

	System::ConstPtr _ms = System::ConstPtr(new JumpConditionSerialization1::MockSystem());	
	JointConfigurationSensorPtr js_s = JointConfigurationSensorPtr(new JointConfigurationSensor()); 

	js_s->setSystem(_ms);

	// Mocked description returned by jump condition
	MockDescriptionTreeNode* _jc_node = new MockDescriptionTreeNode;
	DescriptionTreeNode::Ptr jc_node(_jc_node);

	EXPECT_CALL(*_jc_node, getType()).WillRepeatedly(Return("JumpCondition"));
	EXPECT_CALL(*_jc_node, getAttributeString(_, _))
		.WillRepeatedly(DoAll(SetArgReferee<1>(""),Return(true)));

	//-------
	// TEST ControlMode

	// Mocked tree factory
	MockDescriptionTree* _tree = new MockDescriptionTree;
	MockDescriptionTree::Ptr tree(_tree);

	JumpCondition* _jc1 = new JumpCondition;
	JumpCondition::Ptr jc1(_jc1);	

	MockDescriptionTreeNode::Ptr cm1_node_faulty(new MockDescriptionTreeNode);
	EXPECT_CALL(*cm1_node_faulty, setAttributeString(_,_))
		.Times(AtLeast(0)); 
	// assert throwing of exception if no sensor has been set
	EXPECT_CALL(*_tree, createNode("JumpCondition"))
		.WillOnce(Return(cm1_node_faulty));

	ASSERT_ANY_THROW(jc1->serialize(tree));
	
	jc1->setSensor(js_s);

	// this will be the node "generated" by the tree
	MockDescriptionTreeNode* _jc1_node = new MockDescriptionTreeNode;
	DescriptionTreeNode::Ptr jc1_node(_jc1_node);

	// and one for the sensot
	MockDescriptionTreeNode* _sensor_node = new MockDescriptionTreeNode;
	DescriptionTreeNode::Ptr sensor_node(_sensor_node);

	EXPECT_CALL(*_tree, createNode("JumpCondition"))
	.WillOnce(Return(jc1_node));

	EXPECT_CALL(*_tree, createNode("Sensor"))
	.WillOnce(Return(sensor_node));

	// Expect that exactly one child node = sensor will be added as child node
	EXPECT_CALL(*_jc1_node, addChildNode(_))
	.Times(AtLeast(1));
	EXPECT_CALL(*_sensor_node, addChildNode(_))
	.Times(AtLeast(0));

	// -> some properties (don't care)
	EXPECT_CALL(*_jc1_node, setAttributeString(_,_))
	.Times(AtLeast(0));
	EXPECT_CALL(*_sensor_node, setAttributeString(_,_))
	.Times(AtLeast(0));


	DescriptionTreeNode::Ptr jc_serialized;
	jc_serialized = jc1->serialize(tree);
}

TEST(JumpCondition, Activation) {
	using namespace ha;
	using namespace std;

	//-------
	// Serialized and deserialized sensor

	JumpConditionSerialization1::MockSystem* _ms = new JumpConditionSerialization1::MockSystem();
	System::ConstPtr ms(_ms);

	JointConfigurationSensor* _js_s = new JointConfigurationSensor();
	JointConfigurationSensor::Ptr js_s(_js_s);	
	js_s->setSystem(ms);

	JumpCondition* _jc1 = new JumpCondition;
	JumpCondition::Ptr jc1(_jc1);	
	jc1->setSensor(js_s);

	//The sensor reading of the system - we will mock it as constant
	::Eigen::MatrixXd sensorMat(3,1);
	sensorMat<<1.0,1.0,1.0;
	EXPECT_CALL(*_ms, getConfiguration())
		.WillRepeatedly(Return(sensorMat));

	/////////////////////////////////////////////////
	//test 1: invalid dimensions

	//The goal of the system - we will change it in this test
	::Eigen::MatrixXd goalMat(2,1);
	goalMat<<1.0,2.0;
	jc1->setConstantGoal(goalMat);
	
	EXPECT_ANY_THROW(jc1->isActive());

	goalMat.resize(1,3);
	goalMat<<1.0,2.0,3.0;
	jc1->setConstantGoal(goalMat);
	EXPECT_ANY_THROW(jc1->isActive());

	/////////////////////////////////////////////////
	//test 2: Norms
	jc1->setNorm(JumpCondition::L1);		
	jc1->setEpsilon(0.1);

	goalMat.resize(3,1);
	
	goalMat<<1.1,1.0,1.01;
	jc1->setConstantGoal(goalMat);
	EXPECT_FALSE(jc1->isActive());
		
	goalMat<<1.08,1.0,1.01;
	jc1->setConstantGoal(goalMat);
	EXPECT_TRUE(jc1->isActive());

	//L_INF
	jc1->setNorm(JumpCondition::L_INF);
	goalMat<<1.11,1.0,1.0;
	jc1->setConstantGoal(goalMat);
	EXPECT_FALSE(jc1->isActive());
		
	goalMat<<1.09,1.09,1.09;
	jc1->setConstantGoal(goalMat);
	EXPECT_TRUE(jc1->isActive());
}

