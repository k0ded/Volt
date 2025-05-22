import winreg
import os
import subprocess

engineDirKeyPath = r"Software\\Classes\\Volt.Sandbox"
valueName = "EngineDirectory"

class GenerateProjects:
    @staticmethod
    def Generate():
        engineDirectory = ""

        try:
            registryKey = winreg.OpenKey(winreg.HKEY_CURRENT_USER, engineDirKeyPath, 0, winreg.KEY_READ)
            value, regType = winreg.QueryValueEx(registryKey, valueName)

            engineDirectory = value

            winreg.CloseKey(registryKey)

        except FileNotFoundError:
            print("The specified registry key or value was not found.")
        except PermissionError:
            print("You don't have permission to read this registry key.")

        os.environ["VOLT_PATH_TEMP"] = engineDirectory
        env = os.environ.copy()

        os.chdir('../Sharpmake')

        subprocess.run(["Sharpmake.Application.exe", "/sources('../Game.Main.sharpmake.cs')"])

if __name__ == "__main__":
    os.chdir("Scripts")
    GenerateProjects.Generate()