import xml.etree.ElementTree as ET
import argparse
import os
import sys

def indent_xml(elem, level=0):
    """Adds indentation to an XML element and its children for pretty-printing."""
    i = "\n" + "    " * level  # Four spaces per level
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "    "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for child in elem:
            indent_xml(child, level + 1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if not elem.tail or not elem.tail.strip():
            elem.tail = i

def convert_xml(input_file, output_file):
    try:
        # Normalize file paths for cross-platform compatibility
        input_file = os.path.normpath(input_file)
        output_file = os.path.normpath(output_file)

        # Parse the input XML file
        tree = ET.parse(input_file)
        root = tree.getroot()

        # Ensure the root is <Nebula>
        if root.tag != "Nebula":
            raise ValueError("Expected root tag <Nebula>, but got <{}>".format(root.tag))

        # Find the <Surface> tag within <Nebula>
        surface_tag = root.find("Surface")
        if surface_tag is None:
            raise ValueError("Expected <Surface> tag within <Nebula>, but none was found.")

        # Create a new <Params> element
        params_tag = ET.Element("Params")

        # Iterate through <Param> elements within <Surface>
        for param in surface_tag.findall("Param"):
            name = param.get("name")
            value = param.get("value")

            if name and value:
                # Create a new tag for each name/value pair
                new_tag = ET.Element(name)
                new_tag.text = value
                params_tag.append(new_tag)

        # Remove all existing <Param> elements from <Surface>
        for param in surface_tag.findall("Param"):
            surface_tag.remove(param)

        # Add the new <Params> tag to <Surface>
        surface_tag.append(params_tag)

        # Pretty-print the XML by adding indentation
        indent_xml(root)

        # Write the formatted XML to the output file
        tree.write(output_file, encoding="utf-8", xml_declaration=True)

        print(f"Converted XML written to {output_file}")
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)

def main():
    # Set up command-line argument parsing
    parser = argparse.ArgumentParser(description="Convert <Param> XML nodes into <Params> with child tags.")
    parser.add_argument("input_file", help="Path to the input XML file")
    parser.add_argument("output_file", help="Path to the output XML file")

    # Parse the arguments
    args = parser.parse_args()

    # Call the conversion function with the provided file paths
    convert_xml(args.input_file, args.output_file)

if __name__ == "__main__":
    main()
