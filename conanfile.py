from conans import ConanFile, CMake, tools


class DamnflagsConan(ConanFile):
    name = "damnflags"
    version = "0.1"
    license = "GPLv3"
    author = "DevCodeOne"
    url = ""
    description = "<Description of Damnflags here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"
    requires =  "jsonformoderncpp/3.6.1@vthiery/stable", \
                "spdlog/1.3.1@bincrafters/stable", \
                "clara/1.1.5@bincrafters/stable"

    def build(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = "On"
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="hello")
        self.copy("*hello.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["hello"]

