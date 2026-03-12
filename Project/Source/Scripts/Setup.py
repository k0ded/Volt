from re import sub
import sys
import subprocess
import os

from SetupPython import Python

Python.CheckPython()

import colorama

from colorama import Fore
from SetupSharpmake import Sharpmake
from GenerateProjects import GenerateProjects

os.chdir("Scripts")

colorama.init()

print("")
Sharpmake.CheckSharpmake()

sys.stdout.write(Fore.WHITE)

GenerateProjects.Generate()