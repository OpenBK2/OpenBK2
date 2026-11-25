import tkinter
import virtual_file_system_abstract


data_folder_entry: tkinter.Entry = None

mod_folder_entry: tkinter.Entry = None

load_data_button: tkinter.Button = None

file_system: virtual_file_system_abstract = None

search_entry: tkinter.Entry = None

items_all: list = None

items_variable: tkinter.Variable = None

items_list: tkinter.Listbox = None

search_button: tkinter.Button = None

scrollbar: tkinter.Scrollbar = None

searching_items: list = None

select_button: tkinter.Button = None

export_label: tkinter.Label = None

export_entry: tkinter.Entry = None

export_pick_button: tkinter.Button = None

export_unit_folder_bool_var: tkinter.BooleanVar = None

export_unit_folder_check_button: tkinter.Checkbutton = None

export_weapons_bool_var: tkinter.BooleanVar = None

export_weapons_check_button: tkinter.Checkbutton = None

export_selected_item_button: tkinter.Button = None