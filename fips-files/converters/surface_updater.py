import xml.etree.ElementTree as ET
import argparse
import os
import sys
import material_generated
import flatbuffers

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

        builder = flatbuffers.Builder(1024)
        template = builder.CreateString(surface_tag.get("template", ""))

        names = []
        values = []
        params = surface_tag.findall("Params")
        for param in params:
            for child in param:
                param_name = child.tag
                param_value = child.get("value", "")

                name_offset = builder.CreateString(param_name)
                value_offset = builder.CreateString(param_value)
                names.append(name_offset)
                values.append(value_offset)

        material_generated.MaterialResourceStartValueNamesVector(builder, len(names))
        for name in reversed(names):
            builder.PrependUOffsetTRelative(name)
        names_vector = builder.EndVector(len(names))

        material_generated.MaterialResourceStartValuesVector(builder, len(values))
        for value in reversed(values):
            builder.PrependUOffsetTRelative(value)
        values_vector = builder.EndVector(len(values))

        material_generated.MaterialResourceStart(builder)
        material_generated.MaterialResourceAddTemplateName(builder, template)        
        material_generated.MaterialResourceAddValueNames(builder, names_vector)

        material_generated.MaterialResourceAddValues(builder, values_vector)
        material_resource = material_generated.MaterialResourceEnd(builder)
        builder.Finish(material_resource)

        buf = builder.Output()
        with open(output_file, "wb") as f:
            f.write(buf)

        print(f"Converted XML written to {output_file} flatbuffers")
    except Exception as e:
        print(f"Error processing file {input_file}: {e}", file=sys.stderr)

def process_directory(input_dir):
    """Recursively processes all XML files in a directory structure."""
    for root, _, files in os.walk(input_dir):
        for file in files:
            if file.endswith(".sur"):
                input_file = os.path.join(root, file)
                output_file = os.path.splitext(input_file)[0] + ".namat"  # Overwrite the original file
                print(f"Processing {input_file}...")
                convert_xml(input_file, output_file)

def main():
    # Set up command-line argument parsing
    parser = argparse.ArgumentParser(description="Convert <Param> XML nodes into <Params> with child tags.")
    parser.add_argument(
        "--file", 
        help="Path to a single input XML file to process (mutually exclusive with --dir)"
    )
    parser.add_argument(
        "--dir", 
        help="Path to a directory containing XML files to process recursively (mutually exclusive with --file)"
    )

    # Parse the arguments
    args = parser.parse_args()

    if args.file and args.dir:
        print("Error: You must specify either --file or --dir, not both.", file=sys.stderr)
        sys.exit(1)

    if args.file:
        # Process a single file
        input_file = os.path.normpath(args.file)
        output_file = os.path.splitext(input_file)[0] + ".namat"  # Overwrite the original file
        convert_xml(input_file, output_file)

    elif args.dir:
        # Process all files in a directory recursively
        input_dir = os.path.normpath(args.dir)
        process_directory(input_dir)

    else:
        print("Error: You must specify either --file or --dir.", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
