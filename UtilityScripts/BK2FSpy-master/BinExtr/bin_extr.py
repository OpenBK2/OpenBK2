import pathlib
import tkinter

from folder_system import FolderSystem
from pak_loader import PakLoader
from console_logger import ConsoleLogger
from virtual_file_system import VirtualFileSystem
from tkinter import *
from tkinter import ttk, filedialog, messagebox
import bin_extr_data
import os
# from difflib import SequenceMatcher
# import editdistance
# import Levenshtein
import heapq
import utils


def top_n_similar_items(collection, target, similarity_function, n):
	# Define a priority queue to store the top 20 similar items
	heap = []

	# Iterate through the list and maintain the top 20 most similar items
	for item in collection:
		similarity = similarity_function(item, target)
		if len(heap) < n:
			heapq.heappush(heap, (similarity, item))
		else:
			# If the similarity of the current item is greater than the similarity of the smallest item in the heap,
			# replace the smallest item with the current item
			if similarity > heap[0][0]:
				heapq.heappop(heap)
				heapq.heappush(heap, (similarity, item))

	# Extract the top 20 most similar items from the heap
	top_20 = list(filter(None, [item if similarity > 0 else None for similarity, item in sorted(heap, reverse=False)]))
	return top_20


def similar_strings(a: str, b: str) -> float:
	# return SequenceMatcher(None, a, b).ratio()
	# return Levenshtein.distance(a, b)
	"""dist = editdistance.eval(a, b)
		similarity = 1 - (dist / max(len(a), len(b)))
		return similarity"""
	# still gives the best results:
	return 1 if b in a or a in b else 0



def set_entry_text(entry: Entry, text: str):
	entry.delete(0, END)
	entry.insert(0, text)


def data_folder_select_command():
	folder = filedialog.askdirectory()
	set_entry_text(bin_extr_data.data_folder_entry, folder)


def mod_folder_select_command():
	folder = filedialog.askdirectory()
	set_entry_text(bin_extr_data.mod_folder_entry, folder)


def load_data_command():
	path1 = bin_extr_data.data_folder_entry.get().strip()
	path2 = bin_extr_data.mod_folder_entry.get().strip()

	if not path1 and not path2:
		messagebox.showerror("Error", "You must select a folder(s) to load the data from.")
		return

	data_path = path1
	mod_path = path2
	if not data_path:
		data_path = path2

	paths = list(filter(None, dict.fromkeys([mod_path, data_path])))

	for path in paths:
		if not os.path.exists(path) or not os.path.isdir(path):
			messagebox.showerror("Error", "Invalid path(s) given!")
			return

	logger = ConsoleLogger()

	dir1 = FolderSystem(paths[0])
	pak1 = PakLoader(paths[0], logger)

	bin_extr_data.file_system = VirtualFileSystem(dir1)
	bin_extr_data.file_system.add_system(pak1)

	if len(paths) > 1:
		bin_extr_data.file_system.add_system(FolderSystem(paths[1]))
		bin_extr_data.file_system.add_system(PakLoader(paths[1], logger))

	bin_extr_data.items_all = sorted(bin_extr_data.file_system.get_all_files())
	bin_extr_data.items_variable.set(bin_extr_data.items_all)

	set_items_state(bin_extr_data.searching_items, tkinter.NORMAL)

	messagebox.showinfo('Info', 'Data Loaded!')


def search_filter(event=None):

	if event is None:
		query = bin_extr_data.search_entry.get()
	else:
		query = event.widget.get().strip()

	if not query:
		bin_extr_data.items_variable.set(bin_extr_data.items_all)
		return

	# coll = bin_extr_data.items_all
	# coll.sort(key= lambda x: similar_strings(x, event.widget.get()))
	# coll = coll[10:]
	query = utils.formatted_path(query)
	coll = top_n_similar_items(bin_extr_data.items_all, query, similar_strings, 256)
	bin_extr_data.items_variable.set(coll)


def export_RPGStats_item(file_system: VirtualFileSystem, path: str, out_path: str, export_unit_folder: bool,
						 export_unit_weapons: bool):

	if not os.path.isdir(out_path):
		messagebox.showerror("Error", "Invalid export path given!")
		return

	import bk2_xml_utils

	used_paths = set()
	reader = bk2_xml_utils.VisualObjectReader(file_system, used_paths)
	reader.read_RPGStats(path, os.path.dirname(path), export_unit_weapons)

	for result in reader.result:
		if not bk2_xml_utils.copy_file_to_folder(file_system, result, out_path):
			messagebox.showerror("Error", f"Error copying '{result}' to '{out_path}'")

	if export_unit_folder:
		for file in reader.used_file_paths:
			bk2_xml_utils.copy_file_to_folder(file_system, file, out_path)

	messagebox.showinfo("Info", "Exported!")
	return


def export_single_item(file_system: VirtualFileSystem, path: str, out_path: str):
	if not os.path.isdir(out_path):
		messagebox.showerror("Error", "Invalid export path given!")
		return

	import bk2_xml_utils

	bk2_xml_utils.copy_file_to_folder(file_system, path, out_path)


def items_list_selected(event=None):
	index = event.widget.curselection()
	# messagebox.showinfo("Info", f"Selected item: {bin_extr_data.items_list.get(index)}")
	export_RPGStats_item(bin_extr_data.file_system, bin_extr_data.items_list.get(index), bin_extr_data.export_entry.get(),
						 bin_extr_data.export_unit_folder_bool_var.get(), bin_extr_data.export_weapons_bool_var.get())
	return


# tkinter.DISABLED or tkinter.NORMAL
def set_items_state(items: list, new_state):
	for item in items:
		item.config(state=new_state)


def select_button_pressed():
	selected =  bin_extr_data.items_list.curselection()
	if selected is None or len(selected) < 1:
		messagebox.showerror("Error", "Nothing was selected for exporting")
		return
	export_RPGStats_item(bin_extr_data.file_system, bin_extr_data.items_list.get(selected[0]), bin_extr_data.export_entry.get(),
						 bin_extr_data.export_unit_folder_bool_var.get(), bin_extr_data.export_weapons_bool_var.get())
	return


def export_selected_item_command():
	selected = bin_extr_data.items_list.curselection()
	if selected is None or len(selected) < 1:
		messagebox.showerror("Error", "Nothing was selected for exporting")
		return
	export_single_item(bin_extr_data.file_system, bin_extr_data.items_list.get(selected[0]), bin_extr_data.export_entry.get())
	messagebox.showinfo("Info", "Exported!")
	return


def select_export_folder_command():
	folder = filedialog.askdirectory()
	set_entry_text(bin_extr_data.export_entry, folder)


def main():
	root = Tk()
	root.title('BK2 Bin Extractor')
	root.geometry("800x600")
	root.iconbitmap("icon.ico")
	root.resizable(False, False)

	frame = ttk.Frame(root, padding=10)
	frame.grid()

	ttk.Label(frame, text="Game's Data Folder: ").grid(column=0, row=0, pady=1)
	bin_extr_data.data_folder_entry = ttk.Entry(frame, width=100)
	bin_extr_data.data_folder_entry.grid(column=1, row=0, pady=1)
	ttk.Button(frame, text="...", width=7, command=data_folder_select_command).grid(column=2, row=0, pady=1)
	"""if __debug__:
		bin_extr_data.data_folder_entry.insert(
			0,
			r"C:/Program Files (x86)/Steam/steamapps/common/Blitzkrieg 2 Anthology/Blitzkrieg 2/Data"
		)"""

	ttk.Label(frame, text="Mod Folder: ").grid(column=0, row=1, pady=1)
	bin_extr_data.mod_folder_entry = ttk.Entry(frame, width=100)
	bin_extr_data.mod_folder_entry.grid(column=1, row=1, pady=1)
	ttk.Button(frame, text="...", width=7, command=mod_folder_select_command).grid(column=2, row=1, pady=1)
	"""if __debug__:
		bin_extr_data.mod_folder_entry.insert(
			0,
			r"C:/Program Files (x86)/Steam/steamapps/common/Blitzkrieg 2 Anthology/Blitzkrieg 2/mods/Universal MOD-18 Xitest"
		)"""

	bin_extr_data.load_data_button = ttk.Button(frame, text="Load Data", width=20, command=load_data_command)
	bin_extr_data.load_data_button.grid(column=0, row=2, columnspan=3, pady=1)

	ttk.Label(frame, text="Search for:").grid(column=0, row=3, pady=1)
	bin_extr_data.search_entry = ttk.Entry(frame, width=95)
	bin_extr_data.search_entry.grid(column=1, row=3, columnspan=1, pady=1)
	bin_extr_data.search_entry.bind("<Return>", search_filter)

	bin_extr_data.search_button = ttk.Button(frame, text="Search", width=7, command=search_filter)
	bin_extr_data.search_button.grid(column=2, row=3, pady=1)

	bin_extr_data.items_variable = tkinter.Variable()
	bin_extr_data.items_list = tkinter.Listbox(frame, height=24, width=120, selectmode=tkinter.SINGLE,
											   listvariable=bin_extr_data.items_variable)
	bin_extr_data.items_list.grid(column=0, row=4, columnspan=2, pady=1)
	bin_extr_data.items_list.bind("<Double-Button-1>", items_list_selected)

	scrollbar = bin_extr_data.scrollbar = Scrollbar(frame, orient="vertical")
	scrollbar.config(command=bin_extr_data.items_list.yview)
	scrollbar.grid(row=4, column=2, sticky='ns')
	bin_extr_data.items_list.config(yscrollcommand=scrollbar.set)

	bin_extr_data.export_label = ttk.Label(frame, text="Export folder: ")
	bin_extr_data.export_label.grid(column=0, row=5, pady=1)
	bin_extr_data.export_entry = ttk.Entry(frame, width=95)
	bin_extr_data.export_entry.grid(column=1, row=5, pady=1)
	bin_extr_data.export_pick_button = ttk.Button(frame, text="...", width=7, command=select_export_folder_command)
	bin_extr_data.export_pick_button.grid(column=2, row=5, pady=1)
	"""if __debug__:
		bin_extr_data.export_entry.insert(
			0,
			r"C:/Users/Adam/Desktop/export_test"
		)"""

	bin_extr_data.export_unit_folder_bool_var = tkinter.BooleanVar(value=True)
	bin_extr_data.export_unit_folder_check_button = (
		ttk.Checkbutton(frame, text="Export Unit Folder", variable=bin_extr_data.export_unit_folder_bool_var))
	bin_extr_data.export_unit_folder_check_button.grid(column=0, row=6, pady=1)

	bin_extr_data.export_weapons_bool_var = tkinter.BooleanVar(value=True)
	bin_extr_data.export_weapons_check_button = (
		ttk.Checkbutton(frame, text="Export Unit Weapons", variable=bin_extr_data.export_weapons_bool_var))
	bin_extr_data.export_weapons_check_button.grid(column=1, row=6, pady=1)

	bin_extr_data.export_selected_item_button = ttk.Button(frame, text="Export Selected Item",
														   command=export_selected_item_command)
	bin_extr_data.export_selected_item_button.grid(column=0, row=7, columnspan=1, pady=1)

	bin_extr_data.select_button = ttk.Button(frame, text="Export RPGStats", width=16, command=select_button_pressed)
	bin_extr_data.select_button.grid(column=1, row=7, columnspan=2, pady=1)

	bin_extr_data.searching_items = [bin_extr_data.items_list, bin_extr_data.search_entry, bin_extr_data.search_button,
									 bin_extr_data.select_button, bin_extr_data.export_entry,
									 bin_extr_data.export_pick_button, bin_extr_data.export_label,
									 bin_extr_data.export_weapons_check_button,
									 bin_extr_data.export_unit_folder_check_button,
									 bin_extr_data.export_selected_item_button]
	set_items_state(bin_extr_data.searching_items, tkinter.DISABLED)

	root.mainloop()
	return


if __name__ == "__main__":
	main()