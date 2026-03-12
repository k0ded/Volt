from re import sub
import sys
import subprocess
import os

from SetupPython import Python

def GenerateProjects(filename):
        os.chdir('../')
        if (filename is not None and filename != ""):
                escaped_filename = filename.replace('\\', '\\\\')
                
                if (not os.path.isdir(filename)):
                        filename = os.path.dirname(filename)

                subprocess.call(['setx', 'VOLT_PROJECT', filename])
                subprocess.call(['GenerateProjects.bat', '/project(\'' + escaped_filename + '\')'])
        else:
                subprocess.call(['GenerateProjects.bat'])


Python.CheckPython()

import colorama

from colorama import Fore
from SetupSharpmake import Sharpmake
from SetupVulkan import Vulkan
from SetupRegistryKeys import SetupRegistryKeys
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument('-p', '--project')
parser.add_argument('-s', '--server')

args = parser.parse_args()

os.chdir("Scripts")

colorama.init()

if not args.server:
        Vulkan.CheckVulkan()

print("")
Sharpmake.CheckSharpmake()

sys.stdout.write(Fore.WHITE)

SetupRegistryKeys.Setup()

GenerateProjects(args.project)