#!/bin/python3

import json
import re
from argparse import ArgumentParser


Cooddy_name_pattern = re.compile(r"^(?P<name>.*)\((?P<mangled_name>[^()]*)\)$")


def getNames(name: str) :
  m = Cooddy_name_pattern.fullmatch(name)
  if m:
    return m.group("name"), m.group("mangled_name")
  return name, name


def transform(utbot_json, coody_json):
  for coody_name, annotation in coody_json.items():
    funcName, mangledName = getNames(coody_name)
    utbot_json[mangledName] = {"name": funcName, "annotation": annotation, "properties": []}


def main():
  parser = ArgumentParser(
              prog='cooddy_annotations.py',
              description='This script transforms .json annotations used by Cooddy static analyzer into annotations understandable by KLEE')
  parser.add_argument('filenames', metavar='Path', nargs='+', help="Cooddy annotation .json file path")
  parser.add_argument('Output', help="Target file path for produced KLEE annotation")
  args = parser.parse_args()
  utbot_json = dict()
  for filename in args.filenames:
    with open(filename) as file:
      j = json.load(file)
    transform(utbot_json, j)
  with open(args.Output, 'w') as file:
    json.dump(utbot_json, file, indent="  ")


if __name__ == "__main__":
  main()
