import bk2_xml_utils
from lxml import objectify
import lxml.etree

def add_reinf_type(map_xml, reinf_href_string: str, player_index: int):
	item = objectify.Element("Item")
	item.attrib["href"] = reinf_href_string
	map_xml.Players.Item[player_index].ReinforcementTypes.append(item)


def add_object_on_map(map_xml, object_href: str, position: tuple[float, float, float], direction: int, player: int,
					frame_index=-1, HP = 1, scriptID=-1, link_with: int=-1):
	item = objectify.Element("Item")
	pos = objectify.Element("Pos")
	pos.x = position[0]
	pos.y = position[1]
	pos.z = position[2]
	item.Pos = pos
	item.Dir = direction
	item.Player = player
	item.ScriptID = scriptID
	item.HP = HP
	item.FrameIndex = frame_index
	link = objectify.Element("Link")
	prev_link = int(map_xml.Objects.Item[-1].Link.LinkID)
	link.LinkID = prev_link + 1
	link.LinkWith = link_with
	link.Intention = "false"
	item.Link = link
	obj = objectify.Element("Object")
	obj.attrib["href"] = object_href
	item.Object = obj
	construtor = objectify.Element("ConstructorProfile")
	construtor.attrib["href"] = ""
	item.ConstructorProfile = construtor
	objectify.deannotate(item)
	lxml.etree.cleanup_namespaces(item)
	map_xml.Objects.append(item)


