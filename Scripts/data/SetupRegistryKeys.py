import winreg
import os

fileTypeRegPath = r"Software\\Classes\\.vtproj"
sandboxRegPath = r"Software\\Classes\\Volt.Sandbox"

class SetupRegistryKeys:
    @staticmethod
    def Setup():
        engineDirectory = os.path.abspath("..\\Engine")
        sandboxLaunchCommand = engineDirectory + "\\Binaries\\Sandbox.exe %1"

        fileTypeKey = winreg.CreateKey(winreg.HKEY_CURRENT_USER, fileTypeRegPath)
        winreg.SetValueEx(fileTypeKey, "", 0, winreg.REG_SZ, "Volt.Sandbox")
        winreg.CloseKey(fileTypeKey)

        sandboxKey = winreg.CreateKey(winreg.HKEY_CURRENT_USER, sandboxRegPath)
        winreg.SetValueEx(sandboxKey, "", 0, winreg.REG_SZ, "Volt Sandbox")
        winreg.SetValueEx(sandboxKey, "EngineDirectory", 0, winreg.REG_SZ, engineDirectory)
        winreg.CloseKey(sandboxKey)

        sandboxCommandKey = winreg.CreateKey(winreg.HKEY_CURRENT_USER, sandboxRegPath + "\\Shell\\Open\\Command")
        winreg.SetValueEx(sandboxCommandKey, "", 0, winreg.REG_SZ, sandboxLaunchCommand)
        winreg.CloseKey(sandboxCommandKey)