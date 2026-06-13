import subprocess
try:
    import pkg_resources
except ImportError:
    subprocess.check_call(['python', '-m', 'pip', 'install', 'setuptools==81.0.0'])
    import pkg_resources

class Python:
    @staticmethod
    def Install(package):
        subprocess.check_call(['python', '-m', 'pip', 'install', package])

    @staticmethod
    def ValidatePackage(package):
        required = { package }
        installed = { pkg.key for pkg in pkg_resources.working_set }
        missing = required - installed

        if (missing):
            Python.Install(package)

    @staticmethod
    def CheckPython():
        Python.ValidatePackage('colorama')