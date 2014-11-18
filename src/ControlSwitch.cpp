#include "hybrid_automaton/ControlSwitch.h"

namespace ha {
	void ControlSwitch::add(const JumpConditionPtr& jump_condition)
	{
		_jump_conditions.push_back(jump_condition);
	}

	const std::vector<JumpConditionPtr>& ControlSwitch::getJumpConditions()
	{
		return _jump_conditions;
	}

	bool ControlSwitch::isActive() const {
		for (std::vector<JumpConditionPtr>::const_iterator it = _jump_conditions.begin(); it != _jump_conditions.end(); ++it) {
			if (!(*it)->isActive())
				return false;
		}
		return true;
	}

	void ControlSwitch::activate(const double& t) {
		for (std::vector<JumpConditionPtr>::const_iterator it = _jump_conditions.begin(); it != _jump_conditions.end(); ++it) {
			(*it)->activate(t);
		}
	}

	void ControlSwitch::deactivate() {
		for (std::vector<JumpConditionPtr>::const_iterator it = _jump_conditions.begin(); it != _jump_conditions.end(); ++it) {
			(*it)->deactivate();
		}
	}

	void ControlSwitch::step(const double& t) {
		for (std::vector<JumpConditionPtr>::const_iterator it = _jump_conditions.begin(); it != _jump_conditions.end(); ++it) {
			(*it)->step(t);
		}
	}

	void ControlSwitch::setName(const std::string& name) {
		_name = name;
	}

	const std::string ControlSwitch::getName() const {
		return _name;
	}

	void ControlSwitch::setSourceControlModeName(const std::string& source) {
		_source_control_mode_name = source;
	}
	
	const std::string ControlSwitch::getSourceControlModeName() const {
		return _source_control_mode_name;
	}

	void ControlSwitch::setTargetControlModeName(const std::string& target) {
		_target_control_mode_name = target;
	}
	const std::string ControlSwitch::getTargetControlModeName() const {
		return _target_control_mode_name;
	}

	DescriptionTreeNode::Ptr ControlSwitch::serialize(const DescriptionTree::ConstPtr& factory) const {
		DescriptionTreeNode::Ptr tree_node = factory->createNode("ControlSwitch");
		tree_node->setAttribute<std::string>(std::string("name"), this->getName());
		tree_node->setAttribute<std::string>(std::string("source"), this->getSourceControlModeName());
		tree_node->setAttribute<std::string>(std::string("target"), this->getTargetControlModeName());
		
		for (std::vector<JumpConditionPtr>::const_iterator it = _jump_conditions.begin(); it != _jump_conditions.end(); ++it) {
			DescriptionTreeNode::Ptr jc_node = factory->createNode("JumpCondition");
			tree_node->addChildNode(jc_node);
		}

		return tree_node;
	}
	
	void ControlSwitch::deserialize(const DescriptionTreeNode::ConstPtr& tree, const System::ConstPtr& system, const HybridAutomaton* ha) {
		if (tree->getType() != "ControlSwitch") {
			HA_THROW_ERROR("ControlSwitch.deserialize", "DescriptionTreeNode must have type 'ControlSwitch', not '" << tree->getType() << "'!");
		}

		tree->getAttribute<std::string>("name", _name, "");
		tree->getAttribute<std::string>("source", _source_control_mode_name, "");
		tree->getAttribute<std::string>("target", _target_control_mode_name, "");
		
		if (_source_control_mode_name == "") {
			HA_THROW_ERROR("ControlSwitch.deserialize", "Source must not be empty!");
		}
		if (_target_control_mode_name == "") {
			HA_THROW_ERROR("ControlSwitch.deserialize", "Target must not be empty!");
		}

		DescriptionTreeNode::ConstNodeList jump_conditions;
		tree->getChildrenNodes("JumpCondition", jump_conditions);

		if (jump_conditions.empty()) {
			HA_THROW_ERROR("ControlSwitch.deserialize", "No jump conditions found!");
		}

		DescriptionTreeNode::ConstNodeList::iterator js_it;
		for (js_it = jump_conditions.begin(); js_it != jump_conditions.end(); ++js_it) {
			JumpCondition::Ptr js(new JumpCondition);
			js->deserialize(*js_it, system, ha);
			this->add(js);
		}
	}

}