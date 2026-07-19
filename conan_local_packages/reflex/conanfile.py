from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.scm import Git 
from conan.tools.files import copy
from conan.tools.build import check_min_cppstd
import os


class corralRecipe(ConanFile):
    name = "reflex"
    version = "6.3"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def validate(self):
        check_min_cppstd(self, 20)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/Genivia/RE-flex", target=".")
        git.checkout("v6.3.0")

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "COPYING", self.source_folder, os.path.join(self.package_folder, "licenses"))
        # rmdir(self, os.path.join(self.package_folder, "lib"))
        # rmdir(self, os.path.join(self.package_folder, "share"))

    def package_info(self):
        if self.options.shared:
            self.cpp_info.libs = ["reflex_shared_lib"]
        else:
            self.cpp_info.libs = ["reflex_static_lib"]

        self.cpp_info.set_property("cmake_file_name", "Reflex")
        self.cpp_info.set_property("cmake_target_name", "Reflex::ReflexLib")
        self.cpp_info.set_property("pkg_config_name", "reflex")

    # def package_info(self):
    #     # For header-only packages, libdirs and bindirs are not used
    #     # so it's necessary to set those as empty.
    #     # self.cpp_info.bindirs = []
    #     # self.cpp_info.libdirs = []
    #     self.cpp_info.libs = ["reflex_static_lib"]
    #     self.cpp_info.libs = ["ReflexLib"]
    #     self.cpp_info.set_property("cmake_file_name", "Reflex")
    #     self.cpp_info.set_property("cmake_target_name", "Reflex::ReflexLib")
    #
    #     self.cpp_info.set_property("pkg_config_name", "reflex")
    #     # self.cpp_info.set_property("cmake_target_name", "Reflex::ReflexLibStatic")
    #     # self.cpp_info.set_property("pkg_config_name", "ReflexLib")

    # def package_id(self):
    #     self.info.clear()
