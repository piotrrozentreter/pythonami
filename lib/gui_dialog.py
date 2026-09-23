# Layer 2 modal helper for guicreator / gui_intuition forms.
# See highamigaassembler docs/GUI_PYTHONAMI_API.md (mirrored contract).
# No classes; Amiga-only when used with load_library.

GUI_EVT_CLOSE = 1
GUI_EVT_BUTTON = 2


def snapshot_fields(gui, field_ids):
    # field_ids: list of [kind, id] where kind is "edit", "check", or "list".
    fields = {}
    i = 0
    while i < len(field_ids):
        kind = field_ids[i][0]
        wid = field_ids[i][1]
        if kind == "edit":
            fields[wid] = gui.get_edit_text(wid)
        if kind == "check":
            fields[wid] = gui.get_checkbox(wid)
        if kind == "list":
            fields[wid] = gui.get_list_selected(wid)
        i = i + 1
    return fields


def run_modal(gui):
    # Returns dict: ok (bool), button (int), fields (dict).
    return run_modal_fields(gui, [])


def run_modal_fields(gui, field_ids):
    while 1:
        evt = gui.wait_event()
        if evt == GUI_EVT_CLOSE:
            fields = snapshot_fields(gui, field_ids)
            gui.close_window()
            return {"ok": False, "button": 0, "fields": fields}
        if evt == GUI_EVT_BUTTON:
            eid = gui.get_event_id()
            fields = snapshot_fields(gui, field_ids)
            gui.close_window()
            return {"ok": True, "button": eid, "fields": fields}
