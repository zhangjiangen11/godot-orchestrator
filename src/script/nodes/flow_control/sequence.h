// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef ORCHESTRATOR_SCRIPT_NODE_SEQUENCE_H
#define ORCHESTRATOR_SCRIPT_NODE_SEQUENCE_H

#include "script/nodes/editable_pin_node.h"

/// A node that executes each output execution pin in sequential order.
class OScriptNodeSequence : public OScriptEditablePinNode
{
    ORCHESTRATOR_NODE_CLASS(OScriptNodeSequence, OScriptEditablePinNode);
    static void _bind_methods() { }

public:
    enum InsertPosition
    {
        Before,
        After
    };

protected:
    int _steps{ 2 };  //! The number of sequence steps

    //~ Begin Wrapped Interface
    void _get_property_list(List<PropertyInfo>* r_list) const;
    bool _get(const StringName& p_name, Variant& r_value) const;
    bool _set(const StringName& p_name, const Variant& p_value);
    //~ End Wrapped Interface

public:
    //~ Begin OScriptNode Interface
    void allocate_default_pins() override;
    String get_tooltip_text() const override;
    String get_node_title() const override;
    String get_node_title_color_name() const override { return "flow_control"; }
    String get_icon() const override;
    OScriptNodeInstance* instantiate() override;
    //~ End OScriptNode Interface

    //~ Begin OScriptEditablePinNode Interface
    void add_dynamic_pin() override;
    bool can_add_dynamic_pin() const override;
    bool can_remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) const override;
    void remove_dynamic_pin(const Ref<OScriptNodePin>& p_pin) override;
    String get_pin_prefix() const override { return "then"; }
    //~ End OScriptEditablePinNode Interface

    // Get the number of configured steps
    int get_steps() const { return _steps; }
};

#endif  // ORCHESTRATOR_SCRIPT_NODE_SEQUENCE_H
