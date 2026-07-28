import docx
from docx.oxml.ns import qn
from docx.opc.constants import RELATIONSHIP_TYPE

def copy_element_with_media(element, sub_doc, master_doc):
    """Copies an XML element from sub_doc to master_doc, transferring all media relationships."""
    # Find all elements with r:embed or r:id or r:link attributes
    for node in element.iter():
        for attr_qn in [qn('r:embed'), qn('r:id'), qn('r:link')]:
            if attr_qn in node.attrib:
                old_rId = node.attrib[attr_qn]
                if old_rId in sub_doc.part.rels:
                    rel = sub_doc.part.rels[old_rId]
                    target_part = rel.target_part
                    
                    # Check if target_part is an image / media part
                    if "image" in rel.reltype or "media" in rel.target_ref:
                        # Register part in master_doc.part
                        new_rId = master_doc.part.relate_to(target_part, rel.reltype)
                        node.attrib[attr_qn] = new_rId
                        print(f"  [MEDIA TRANSFERRED] {old_rId} -> {new_rId} ({target_part.partname})")

master = docx.Document('CNCTA/reference1.docx')
master.element.body.clear()

sub = docx.Document('CNCTA/output/temp_chap_03.docx') if os.path.exists('CNCTA/output/temp_chap_03.docx') else None
