from lxml import etree, objectify


def child(parent, tag):
    # An empty optional collection may be absent from an editor-saved template.
    element = parent.find(tag)
    return element if element is not None else etree.SubElement(parent, tag)


def add_reinf_type(player, href):
    etree.SubElement(child(player, "ReinforcementTypes"), "Item", href=href)


def add_object_on_map(objects, href, position, link_id):
    item = objectify.SubElement(objects, "Item")
    pos = objectify.SubElement(item, "Pos")
    pos.x, pos.y, pos.z = position
    item.Dir = 32768
    item.Player = 0
    item.ScriptID = -1
    item.HP = 1
    item.FrameIndex = -1
    link = objectify.SubElement(item, "Link")
    link.LinkID = link_id
    link.LinkWith = -1
    link.Intention = "false"
    etree.SubElement(item, "Object", href=href)
    etree.SubElement(item, "ConstructorProfile", href="")
