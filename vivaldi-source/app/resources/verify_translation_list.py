# Copyright (c) 2016 Vivaldi Technologies AS. All rights reserved

import sys, os
import argparse
import subprocess

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__),
                            "..", "..", "chromium", "tools", "grit")))

from grit import grd_reader
#from grit.node import base
from grit.node import empty
from grit.node import include
from grit.node import structure
from grit.node import message
from grit.node import node_io
from grit.node import misc
from grit import util
from grit import tclib

def main():

  argparser = argparse.ArgumentParser()
  argparser.add_argument("--messages", required=True)
  argparser.add_argument("--stamp-file", required=True)

  argparser.add_argument("-D", action="append", dest="defines", default=[])
  # grit build also supports '-E KEY=VALUE', support that to share command
  # line flags. Dummy parameter in this script
  argparser.add_argument("-E", action="append", dest="build_env", default=[])
  # grit adds -t android when cross-compiling. Dummy parameter in this script.
  argparser.add_argument("-t", action="append", dest="build_env", default=[])
  argparser.add_argument("grd_file")

  options = argparser.parse_args()

  defines = {}
  for define in options.defines:
    name, val = util.ParseDefine(define)
    defines[name] = val

  node_list = {}
  # Don't need to loop platforms, so using win32 as platform
  resources = grd_reader.Parse(options.grd_file, defines = defines, target_platform="win32")
  resources.SetOutputLanguage('en')
  resources.UberClique().keep_additional_translations_ = True
  resources.RunGatherers()

  for node in resources:
    node.unique_id = None
    if "name" in node.attrs and node.GetCliques():
      node.unique_id = "{}.{}.{}".format(node.attrs["name"], node.GetCliques()[0].GetId(),node.attrs["desc"])

  with open(options.messages, "r") as f:
    message_list=[x.strip() for x in f.readlines()]

  missing_entries = []
  for node in resources:
    if node.unique_id and node.unique_id not in node_list:
      if (not isinstance(node, message.MessageNode) or
          not node.IsTranslateable() or
          "name" not in node.attrs):
        continue
      if message_list and node.attrs["name"] not in message_list:
        missing_entries.append(node.attrs['name'])
        print(f"{options.grd_file}: String {node.attrs['name']}: Missing from ids.txt export list")
      node_list[node.unique_id] = node

  if missing_entries:
      print("=============================")
      print("\n".join(sorted(missing_entries)))
      print("=============================")
      sys.exit(-1)

  with open(options.stamp_file, "w") as f:
    pass; # Just touch

if __name__ == "__main__":
  main()
