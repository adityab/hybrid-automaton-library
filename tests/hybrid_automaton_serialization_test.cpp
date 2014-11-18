#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <string>

#include "hybrid_automaton/HybridAutomaton.h"
#include "hybrid_automaton/DescriptionTreeNode.h"
#include "hybrid_automaton/tests/MockDescriptionTree.h"
#include "hybrid_automaton/tests/MockDescriptionTreeNode.h"

using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using ::testing::AtLeast;
using ::testing::_;

using namespace ::ha;

TEST(HybridAutomatonSerialization, setAttributeVector) {
	using namespace ha;
	using ::testing::Return;

	MockDescriptionTreeNode mock_dtn;

	::Eigen::MatrixXd goal(5,1);
	goal << 1.,
		2.,
		3.,
		4.,
		5.;
	std::string goal_exp = "[5,1]1;2;3;4;5";

	EXPECT_CALL(mock_dtn, setAttributeString(std::string("goal"), goal_exp));

	mock_dtn.setAttribute<::Eigen::MatrixXd>("goal", goal);
}

TEST(HybridAutomatonSerialization, getAttributeVector) {
	using namespace ha;

	MockDescriptionTreeNode mock_dtn;


	std::string goal_exp = "[5,1]1;2;3;4;5";
	EXPECT_CALL(mock_dtn, getAttributeString(std::string("goal"), _)).WillOnce(DoAll(SetArgReferee<1>(goal_exp), Return(true)));

	::Eigen::MatrixXd goal_result(1,1);
	goal_result << 1.;
	mock_dtn.getAttribute<::Eigen::MatrixXd>("goal", goal_result);

	::Eigen::MatrixXd goal_true(5,1);
	goal_true << 1., 2., 3., 4., 5.;
	EXPECT_EQ(goal_result, goal_true);

	::Eigen::MatrixXd goal_false(5,1);
	goal_false << 2., 2., 2., 2., 2.;
	EXPECT_NE(goal_result, goal_false);
}

TEST(HybridAutomatonSerialization, setAttributeMatrix) {
	using namespace ha;
	using ::testing::Return;

	MockDescriptionTreeNode mock_dtn;

	::Eigen::MatrixXd goal(5,2);
	goal << 1.,2.,3.,4.,5, 6.,7.,8.,9.,10.;
	std::string goal_exp = "[5,2]1,2;3,4;5,6;7,8;9,10";

	EXPECT_CALL(mock_dtn, setAttributeString(std::string("goal"), goal_exp));

	mock_dtn.setAttribute<::Eigen::MatrixXd>("goal", goal);
}

TEST(HybridAutomatonSerialization, getAttributeMatrix) {
	using namespace ha;

	MockDescriptionTreeNode mock_dtn;


	std::string goal_exp = "[5,2]1,2;3,4;5,6;7,8;9,10";
	EXPECT_CALL(mock_dtn, getAttributeString(std::string("goal"), _)).WillOnce(DoAll(SetArgReferee<1>(goal_exp), Return(true)));

	::Eigen::MatrixXd goal_result(1,1);
	goal_result << 1.;
	mock_dtn.getAttribute<::Eigen::MatrixXd>("goal", goal_result);

	::Eigen::MatrixXd goal_true(5,2);
	goal_true << 1.,2.,3.,4.,5, 6.,7.,8.,9.,10.;
	EXPECT_EQ(goal_result, goal_true);

	::Eigen::MatrixXd goal_false(5,2);
	goal_false << 2., 2., 2., 2., 2., 2., 2., 2., 2., 2.;
	EXPECT_NE(goal_result, goal_false);
}

// --------------------------------------------

class MockSerializableControlMode : public ha::ControlMode {
public:
	typedef boost::shared_ptr<MockSerializableControlMode> Ptr;

	MOCK_CONST_METHOD1(serialize, DescriptionTreeNode::Ptr (const DescriptionTree::ConstPtr& factory) );
	MOCK_CONST_METHOD1(deserialize, void (const ha::DescriptionTreeNode::ConstPtr& tree) );
};

TEST(HybridAutomaton, Serialization) {
	using namespace ha;
	using namespace std;

	MockDescriptionTree::Ptr tree(new MockDescriptionTree);

	//-------
	string ctrlType("MockSerializableController");
	string ctrlSetType("MockSerializableControlSet");

	//-------
	// Serialized and deserialized ControlMode
	MockSerializableControlMode::Ptr cm1(new MockSerializableControlMode);	
	cm1->setName("myCM1");

	// Mocked node returned by control mode
	MockDescriptionTreeNode::Ptr cm1_node(new MockDescriptionTreeNode);
	EXPECT_CALL(*cm1_node, getType()).WillRepeatedly(Return("ControlMode"));
	EXPECT_CALL(*cm1_node, getAttributeString(_, _))
		.WillRepeatedly(DoAll(SetArgReferee<1>(""),Return(true)));

	EXPECT_CALL(*cm1, serialize(_))
		.WillRepeatedly(Return(cm1_node));

	//-------
	// Serialize HybridAutomaton
	HybridAutomaton ha;
	ha.setName("myHA");
	ha.addControlMode(cm1);

	// this will be the node "generated" by the tree
	MockDescriptionTreeNode::Ptr ha_node(new MockDescriptionTreeNode);

	EXPECT_CALL(*tree, createNode("HybridAutomaton"))
		.WillOnce(Return(ha_node));

	// asserts on what serialization of HA will do
	// -> child node
	EXPECT_CALL(*ha_node, addChildNode(boost::dynamic_pointer_cast<DescriptionTreeNode>(cm1_node)))
		.WillOnce(Return());
	// -> some properties (don't care)
	EXPECT_CALL(*ha_node, setAttributeString(_,_))
		.Times(AtLeast(0));

	DescriptionTreeNode::Ptr ha_serialized;
	ha_serialized = ha.serialize(tree);

}


// ------------------------------------------------

// ----------------------------------
namespace DeserializationOnlyControlMode {

class MockRegisteredController : public ha::Controller {
public:
	MockRegisteredController() : ha::Controller() {
	}

	//MOCK_METHOD0(deserialize, void (const DescriptionTreeNode::ConstPtr& tree) );
	MOCK_CONST_METHOD0(getName, std::string () );

	HA_CONTROLLER_INSTANCE(node, system) {
		Controller::Ptr ctrl(new MockRegisteredController);
		//EXPECT_CALL(*ctrl, getType())
		//	.WillRepeatedly(Return("DeserializationOnlyControlModeMockRegisteredController"));
		//EXPECT_CALL(*ctrl, getName())
		//	.WillRepeatedly(Return("MyCtrl1"));
		return ctrl;
	}
};

HA_CONTROLLER_REGISTER("DeserializationOnlyControlModeMockRegisteredController", MockRegisteredController)
// Attention: Only use this controller in ONE test and de-register 
// in this test. Otherwise you might have weird side effects with other tests

}

TEST(HybridAutomaton, DeserializationOnlyControlMode) {
	DescriptionTreeNode::ConstNodeList cm_list;
	DescriptionTreeNode::ConstNodeList cset_list;
	DescriptionTreeNode::ConstNodeList ctrl_list;

	ha::MockDescriptionTree::Ptr tree;
	ha::MockDescriptionTreeNode::Ptr cm1_node;
	ha::MockDescriptionTreeNode::Ptr cset1_node;
	ha::MockDescriptionTreeNode::Ptr ctrl_node;
	ha::MockDescriptionTreeNode::Ptr ha_node;

	// --
	ctrl_node.reset(new MockDescriptionTreeNode);
	EXPECT_CALL(*ctrl_node, getType())
		.WillRepeatedly(Return("Controller"));
	EXPECT_CALL(*ctrl_node, getAttributeString(std::string("name"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("MyCtrl1"),Return(true)));
	EXPECT_CALL(*ctrl_node, getAttributeString(std::string("type"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("DeserializationOnlyControlModeMockRegisteredController"),Return(true)));

	ctrl_list.push_back(ctrl_node);

	// --
	cset1_node.reset(new MockDescriptionTreeNode);
	EXPECT_CALL(*cset1_node, getType())
		.WillRepeatedly(Return("ControlSet"));
	EXPECT_CALL(*cset1_node, getAttributeString(std::string("name"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("CS1"),Return(true)));
	EXPECT_CALL(*cset1_node, getChildrenNodes(std::string("Controller"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>(ctrl_list),Return(true)));

	cset_list.push_back(cset1_node);

	// --
	// Mocked ControlMode nodes
	tree.reset(new MockDescriptionTree);

	cm1_node.reset(new MockDescriptionTreeNode);
	EXPECT_CALL(*cm1_node, getType())
		.WillRepeatedly(Return("ControlMode"));
	EXPECT_CALL(*cm1_node, getAttributeString(std::string("name"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("CM1"),Return(true)));
	EXPECT_CALL(*cm1_node, getChildrenNodes(std::string("ControlSet"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>(cset_list),Return(true)));

	cm_list.push_back(cm1_node);

	//----------
	ha_node.reset(new MockDescriptionTreeNode);

	EXPECT_CALL(*ha_node, getType())
		.WillRepeatedly(Return("HybridAutomaton"));
	EXPECT_CALL(*ha_node, getAttributeString(std::string("name"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("MyHA"),Return(true)));
	
	EXPECT_CALL(*ha_node, getChildrenNodes(std::string("ControlMode"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>(cm_list),Return(true)));
	EXPECT_CALL(*ha_node, getChildrenNodes(std::string("ControlSwitch"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>(DescriptionTreeNode::ConstNodeList()),Return(true)));

}


// =============================================================

namespace DeserializationControlSet {

class MockRegisteredControlSet : public ha::ControlSet {
public:
	MockRegisteredControlSet() : ha::ControlSet() {
	}

	//MOCK_METHOD0(deserialize, void (const DescriptionTreeNode::ConstPtr& tree) );
	MOCK_CONST_METHOD0(getName, const std::string () );

	HA_CONTROLSET_INSTANCE(node, system) {
		ControlSet::Ptr ctrlSet(new MockRegisteredControlSet);
		return ctrlSet;
	}
};

HA_CONTROLSET_REGISTER("DeserializationControlSetMockRegisteredControlSet", MockRegisteredControlSet)
// Attention: Only use this controller in ONE test and de-register 
// in this test. Otherwise you might have weird side effects with other tests

}

class HybridAutomatonDeserializationTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		// Mocked ControlMode nodes
		tree.reset(new MockDescriptionTree);

		// --
		ctrl_node.reset(new MockDescriptionTreeNode);
		EXPECT_CALL(*ctrl_node, getType())
			.WillRepeatedly(Return("Controller"));
		EXPECT_CALL(*ctrl_node, getAttributeString(std::string("name"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>("MyCtrl1"),Return(true)));
		EXPECT_CALL(*ctrl_node, getAttributeString(std::string("type"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>("DeserializationOnlyControlModeMockRegisteredController"),Return(true)));
		ctrl_list.push_back(ctrl_node);

		// --
		cset1_node.reset(new MockDescriptionTreeNode);
		EXPECT_CALL(*cset1_node, getType())
			.WillRepeatedly(Return("ControlSet"));
		EXPECT_CALL(*cset1_node, getAttributeString(std::string("type"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>("DeserializationControlSetMockRegisteredControlSet"),Return(true)));
		EXPECT_CALL(*cset1_node, getAttributeString(std::string("name"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>("CS1"),Return(true)));
		EXPECT_CALL(*cset1_node, getChildrenNodes(std::string("Controller"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>(ctrl_list),Return(true)));
		cset_list.push_back(cset1_node);

		// --
		cm1_node.reset(new MockDescriptionTreeNode);
		EXPECT_CALL(*cm1_node, getType())
			.WillRepeatedly(Return("ControlMode"));
		EXPECT_CALL(*cm1_node, getAttributeString(std::string("name"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>("CM1"),Return(true)));
		EXPECT_CALL(*cm1_node, getChildrenNodes(std::string("ControlSet"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>(cset_list),Return(true)));

		cm2_node.reset(new MockDescriptionTreeNode);
		EXPECT_CALL(*cm2_node, getType())
			.WillRepeatedly(Return("ControlMode"));
		EXPECT_CALL(*cm2_node, getAttributeString(std::string("name"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>("CM2"),Return(true)));
		EXPECT_CALL(*cm2_node, getChildrenNodes(std::string("ControlSet"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>(cset_list),Return(true)));

		cm_list.push_back(cm1_node);
		cm_list.push_back(cm2_node);

		// --
		cs_node.reset(new MockDescriptionTreeNode);
		EXPECT_CALL(*cs_node, getType())
			.WillRepeatedly(Return("ControlSwitch"));
		EXPECT_CALL(*cs_node, getAttributeString(std::string("name"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>(""),Return(true)));

		cs_list.push_back(cs_node);

		// --
		js_node.reset(new MockDescriptionTreeNode);
		EXPECT_CALL(*js_node, getType())
			.WillRepeatedly(Return("JumpCondition"));
		EXPECT_CALL(*js_node, getAttributeString(std::string("name"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>(""),Return(true)));
		EXPECT_CALL(*js_node, getAttributeString(std::string("type"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>("JumpCondition"),Return(true)));

		js_list.push_back(js_node);

		//----------
		ha_node.reset(new MockDescriptionTreeNode);

		EXPECT_CALL(*ha_node, getType())
			.WillRepeatedly(Return("HybridAutomaton"));
		EXPECT_CALL(*ha_node, getAttributeString(std::string("name"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>("MyHA"),Return(true)));
		
		EXPECT_CALL(*ha_node, getChildrenNodes(std::string("ControlMode"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>(cm_list),Return(true)));
		EXPECT_CALL(*ha_node, getChildrenNodes(std::string("ControlSwitch"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>(cs_list),Return(true)));

		EXPECT_CALL(*cs_node, getChildrenNodes(std::string("JumpCondition"), _))
			.WillRepeatedly(DoAll(SetArgReferee<1>(js_list),Return(true)));
	}

	virtual void TearDown() {
		cm_list.clear();
		cs_list.clear();
		cs_list.clear();
		ctrl_list.clear();
	}

	MockDescriptionTreeNode::ConstNodeList cm_list;
	MockDescriptionTreeNode::ConstNodeList ctrl_list;
	MockDescriptionTreeNode::ConstNodeList cset_list;
	MockDescriptionTreeNode::ConstNodeList cs_list;
	MockDescriptionTreeNode::ConstNodeList js_list;

	MockDescriptionTree::Ptr tree;

	MockDescriptionTreeNode::Ptr cm1_node;
	MockDescriptionTreeNode::Ptr cm2_node;
	MockDescriptionTreeNode::Ptr cset1_node;
	MockDescriptionTreeNode::Ptr ctrl_node;
	MockDescriptionTreeNode::Ptr ha_node;
	MockDescriptionTreeNode::Ptr cs_node;
	MockDescriptionTreeNode::Ptr js_node;

};


TEST_F(HybridAutomatonDeserializationTest, DeserializationSuccessful) {
	using namespace ha;
	using namespace std;

	// Mocked ControlSwitch node
	EXPECT_CALL(*cs_node, getAttributeString(std::string("source"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("CM1"),Return(true)));
	EXPECT_CALL(*cs_node, getAttributeString(std::string("target"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("CM2"),Return(true)));

	// this will be the node "generated" by the tree
	HybridAutomaton ha;
	ha.deserialize(ha_node, System::Ptr());
	
}

TEST_F(HybridAutomatonDeserializationTest, DeserializationUnsuccessful1) {
	using namespace ha;
	using namespace std;

	// Mocked ControlSwitch node
	EXPECT_CALL(*cs_node, getAttributeString(std::string("source"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("CM1"),Return(true)));

	// setting inexistent target
	EXPECT_CALL(*cs_node, getAttributeString(std::string("target"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("Fantasia"),Return(true)));

	// this will be the node "generated" by the tree
	HybridAutomaton ha;
	
	ASSERT_ANY_THROW(ha.deserialize(ha_node, System::Ptr()));
	
}

TEST_F(HybridAutomatonDeserializationTest, DeserializationUnsuccessful2) {
	using namespace ha;
	using namespace std;

	// Mocked ControlSwitch node
	// setting inexistent target
	EXPECT_CALL(*cs_node, getAttributeString(std::string("source"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("Fantasia"),Return(true)));

	// setting inexistent target
	EXPECT_CALL(*cs_node, getAttributeString(std::string("target"), _))
		.WillRepeatedly(DoAll(SetArgReferee<1>("CM2"),Return(true)));

	// this will be the node "generated" by the tree
	HybridAutomaton ha;
	
	ASSERT_ANY_THROW(ha.deserialize(ha_node, System::Ptr()));
	
}