#!/usr/bin/env python3
"""Convert legacy NebulaT particle XML (.attributes) to JSON .par format.

Example:
    python3 convert_particle_attributes_to_par.py \
        /path/to/Build_dust.attributes \
        /path/to/Build_dust.par
"""

from __future__ import annotations

import argparse
import json
import pathlib
import xml.etree.ElementTree as ET
from typing import Any, Dict, List


BOOL_KEYS = {
    "Looping",
    "StretchToStart",
    "RenderOldestFirst",
    "ViewAngleFade",
    "Billboard",
    "RandomizeRotation",
}

INT_KEYS = {
    "StretchDetail",
    "AnimPhases",
}

VEC4_KEYS = {
    "WindDirection",
}

# Curves commonly used by the JSON format. Missing keys are backfilled with defaults.
DEFAULT_CURVES = {
    "Red": [1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "Green": [1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "Blue": [1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "Alpha": [1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "EmissionFrequency": [10.0, 10.0, 10.0, 10.0, 0.0, 10.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "LifeTime": [1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "StartVelocity": [10.0, 10.0, 10.0, 10.0, 0.0, 10.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "RotationVelocity": [0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "Size": [1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "SpreadMin": [10.0, 10.0, 10.0, 10.0, 0.0, 10.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "SpreadMax": [10.0, 10.0, 10.0, 10.0, 0.0, 10.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "AirResistance": [1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "VelocityFactor": [1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "Mass": [0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "TimeManipulator": [0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
    "Alignment0": [0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.33, 0.66, 0.0, 0.0, 0.0],
}


def _parse_csv_floats(value: str, expected_count: int | None = None) -> List[float]:
    values = [float(x.strip()) for x in value.split(",") if x.strip()]
    if expected_count is not None and len(values) != expected_count:
        raise ValueError(f"Expected {expected_count} values, got {len(values)} in '{value}'")
    return values


def _parse_bool(value: str) -> bool:
    return value.strip().lower() in {"true", "1", "yes"}


def _particle_name(raw_name: str) -> str:
    return raw_name.split("/")[-1] if "/" in raw_name else raw_name


def _curve_from_xml(element: ET.Element) -> List[float]:
    values = _parse_csv_floats(element.attrib.get("Values", "0,0,0,0"), expected_count=4)
    limits = _parse_csv_floats(element.attrib.get("Limits", "0,1"), expected_count=2)
    pos0 = float(element.attrib.get("Pos0", "0.33"))
    pos1 = float(element.attrib.get("Pos1", "0.66"))
    function = float(element.attrib.get("Function", "0"))
    frequency = float(element.attrib.get("Frequency", "0"))
    amplitude = float(element.attrib.get("Amplitude", "0"))

    # JSON curve layout used in .par files:
    # [v0, v1, v2, v3, limitMin, limitMax, pos0, pos1, function, frequency, amplitude]
    return values + limits + [pos0, pos1, function, frequency, amplitude]


def convert_xml_to_par(xml_path: pathlib.Path) -> Dict[str, Any]:
    root = ET.parse(xml_path).getroot()

    model_node_material: Dict[str, str] = {}
    for node in root.findall("./States/ModelNode"):
        node_name = node.attrib.get("name", "")
        model_node_material[node_name] = node.attrib.get("material", "")

    emitters: List[Dict[str, Any]] = []
    for particle in root.findall("./Particles/Particle"):
        raw_name = particle.attrib.get("name", "UnnamedEmitter")
        emitter_name = _particle_name(raw_name)

        emitter: Dict[str, Any] = {
            "name": emitter_name,
            "mesh": particle.attrib.get("mesh", ""),
            "material": model_node_material.get(raw_name, ""),
            "transform": [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
            ],
            "floats": {},
            "bools": {},
            "ints": {},
            "vecs": {},
            "curves": {},
        }

        for child in particle:
            if child.tag == "Values":
                for key, value in child.attrib.items():
                    if key in BOOL_KEYS:
                        emitter["bools"][key] = _parse_bool(value)
                    elif key in INT_KEYS:
                        emitter["ints"][key] = int(float(value))
                    elif key in VEC4_KEYS:
                        emitter["vecs"][key] = _parse_csv_floats(value, expected_count=4)
                    else:
                        emitter["floats"][key] = float(value)
            else:
                emitter["curves"][child.tag] = _curve_from_xml(child)

        for curve_name, curve_default in DEFAULT_CURVES.items():
            emitter["curves"].setdefault(curve_name, curve_default)

        emitters.append(emitter)

    return {"emitters": emitters}


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert NebulaT XML particle attributes to JSON .par")
    parser.add_argument("input", type=pathlib.Path, help="Path to input .attributes XML file")
    parser.add_argument("output", type=pathlib.Path, help="Path to output .par JSON file")
    parser.add_argument(
        "--compact",
        action="store_true",
        help="Write compact JSON (no pretty indent)",
    )
    args = parser.parse_args()

    if not args.input.exists():
        raise FileNotFoundError(f"Input file does not exist: {args.input}")

    data = convert_xml_to_par(args.input)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8") as f:
        if args.compact:
            json.dump(data, f, separators=(",", ":"))
        else:
            json.dump(data, f, indent=4)
        f.write("\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
