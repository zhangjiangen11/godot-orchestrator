// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
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
#include "editor/component_panels/functions_panel.h"

#include "common/callable_lambda.h"
#include "common/scene_utils.h"
#include "editor/script_connections.h"
#include "editor/script_view.h"
#include "plugin/plugin.h"
#include "script/script.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/tree.hpp>

void OrchestratorScriptFunctionsComponentPanel::_show_function_graph(TreeItem* p_item)
{
    // Function name and graph names are synonymous
    const String function_name = p_item->get_text(0);
    emit_signal("show_graph_requested", function_name);
    emit_signal("focus_node_requested", function_name, _script->get_function_node_id(function_name));
    _tree->deselect_all();
}

PackedStringArray OrchestratorScriptFunctionsComponentPanel::_get_existing_names() const
{
    return _script->get_function_names();
}

String OrchestratorScriptFunctionsComponentPanel::_get_tooltip_text() const
{
    return "A function graph allows the encapsulation of functionality for re-use. Function graphs have "
           "a single input with an optional output node. Function graphs have a single execution pin "
           "with multiple input data pins and the result node may return a maximum of one data value to "
           "the caller.\n\n"
           "Functions can be called by selecting the action in the action menu or by dragging the "
           "function from this component view onto the graph area.";
}

String OrchestratorScriptFunctionsComponentPanel::_get_remove_confirm_text(TreeItem* p_item) const
{
    return "Removing a function removes all nodes that participate in the function and any nodes\n"
           "that call that function from the event graphs.";
}

bool OrchestratorScriptFunctionsComponentPanel::_populate_context_menu(TreeItem* p_item)
{
    _context_menu->add_item("Open in Graph", CM_OPEN_FUNCTION_GRAPH);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Rename"), "Rename", CM_RENAME_FUNCTION);
    _context_menu->add_icon_item(SceneUtils::get_editor_icon("Remove"), "Remove", CM_REMOVE_FUNCTION);
    return true;
}

void OrchestratorScriptFunctionsComponentPanel::_handle_context_menu(int p_id)
{
    switch (p_id)
    {
        case CM_OPEN_FUNCTION_GRAPH:
            _show_function_graph(_tree->get_selected());
            break;
        case CM_RENAME_FUNCTION:
            _tree->edit_selected(true);
            break;
        case CM_REMOVE_FUNCTION:
            _confirm_removal(_tree->get_selected());
            break;
    }
}

bool OrchestratorScriptFunctionsComponentPanel::_handle_add_new_item(const String& p_name)
{
    return _view->_create_new_function(p_name).is_valid();
}

void OrchestratorScriptFunctionsComponentPanel::_handle_item_selected()
{
    const TreeItem* item = _tree->get_selected();
    if (item)
    {
        const Ref<OScriptFunction> function = _script->find_function(StringName(item->get_text(0)));
        if (function.is_valid())
        {
            const Ref<OScriptNode> node = function->get_owning_node();
            if (node.is_valid())
                OrchestratorPlugin::get_singleton()->get_editor_interface()->edit_resource(node);
        }
    }
}

void OrchestratorScriptFunctionsComponentPanel::_handle_item_activated(TreeItem* p_item)
{
    _show_function_graph(p_item);
}

bool OrchestratorScriptFunctionsComponentPanel::_handle_item_renamed(const String& p_old_name, const String& p_new_name)
{
    if (_get_existing_names().has(p_new_name))
    {
        _show_notification("A function with the name '" + p_new_name + "' already exists.");
        return false;
    }

    _script->rename_function(p_old_name, p_new_name);
    emit_signal("graph_renamed", p_old_name, p_new_name);
    return true;
}

void OrchestratorScriptFunctionsComponentPanel::_handle_remove(TreeItem* p_item)
{
    // Function name and graph names are synonymous
    const String function_name = p_item->get_text(0);
    emit_signal("close_graph_requested", function_name);

    _script->remove_function(function_name);
}

void OrchestratorScriptFunctionsComponentPanel::_handle_button_clicked(TreeItem* p_item, int p_column, int p_id,
                                                                    int p_mouse_button)
{
    const Vector<Node*> nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(_script);

    OrchestratorScriptConnectionsDialog* dialog = memnew(OrchestratorScriptConnectionsDialog);
    add_child(dialog);
    dialog->popup_connections(p_item->get_text(0), nodes);
}

Dictionary OrchestratorScriptFunctionsComponentPanel::_handle_drag_data(const Vector2& p_position)
{
    Dictionary data;

    TreeItem* selected = _tree->get_selected();
    if (selected)
    {
        data["type"] = "function";
        data["functions"] = Array::make(selected->get_text(0));
    }
    return data;
}

void OrchestratorScriptFunctionsComponentPanel::update()
{
    _clear_tree();

    const Vector<Node*> script_nodes = SceneUtils::find_all_nodes_for_script_in_edited_scene(_script);
    const String base_type = _script->get_instance_base_type();

    for (const Ref<OScriptGraph>& graph : _script->get_graphs())
    {
        if (!(graph->get_flags().has_flag(OScriptGraph::GraphFlags::GF_FUNCTION)))
            continue;

        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, graph->get_graph_name());
        item->set_meta("__name", graph->get_graph_name()); // Used for renames
        item->set_icon(0, SceneUtils::get_editor_icon("MemberMethod"));

        if (SceneUtils::has_any_signals_connected_to_function(graph->get_graph_name(), base_type, script_nodes))
            item->add_button(0, SceneUtils::get_editor_icon("Slot"));
    }

    if (_tree->get_root()->get_child_count() == 0)
    {
        TreeItem* item = _tree->get_root()->create_child();
        item->set_text(0, "No functions defined");
        item->set_selectable(0, false);
        return;
    }

    OrchestratorScriptComponentPanel::update();
}

void OrchestratorScriptFunctionsComponentPanel::_notification(int p_what)
{
    // Godot does not dispatch to parent (shrugs)
    OrchestratorScriptComponentPanel::_notification(p_what);

    if (p_what == NOTIFICATION_READY)
    {
        HBoxContainer* container = _get_panel_hbox();

        Button* override_button = memnew(Button);
        override_button->set_focus_mode(FOCUS_NONE);
        override_button->set_button_icon(SceneUtils::get_editor_icon("Override"));
        override_button->set_tooltip_text("Override a Godot virtual function");
        container->add_child(override_button);

        override_button->connect("pressed", callable_mp_lambda(this, [=, this] { emit_signal("override_function_requested"); }));
    }
}

void OrchestratorScriptFunctionsComponentPanel::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("show_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("close_graph_requested", PropertyInfo(Variant::STRING, "graph_name")));
    ADD_SIGNAL(MethodInfo("graph_renamed", PropertyInfo(Variant::STRING, "old_name"), PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo("focus_node_requested", PropertyInfo(Variant::STRING, "graph_name"), PropertyInfo(Variant::INT, "node_id")));
    ADD_SIGNAL(MethodInfo("override_function_requested"));
}

OrchestratorScriptFunctionsComponentPanel::OrchestratorScriptFunctionsComponentPanel(const Ref<OScript>& p_script, OrchestratorScriptView* p_view)
    : OrchestratorScriptComponentPanel("Functions", p_script), _view(p_view)
{
}
