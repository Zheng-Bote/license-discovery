from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class LicenseDiscoveryRecipe(ConanFile):
    name = "license_discovery"
    version = "0.1.0"
    package_type = "library"

    # Optional metadata
    license = "Apache-2.0"
    author = "ZHENG Robert (robert@hase-zheng.net)"
    url = "https://github.com/robert-zheng/license-discovery"
    description = "A C++23 library to query ClearlyDefined Definitions API for license information."
    topics = ("cpp23", "clearlydefined", "license", "discovery", "spdx")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "include/*"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("cpr/1.11.0")
        self.requires("nlohmann_json/3.11.3")
        self.requires("argparse/3.0")
        self.requires("catch2/3.8.0", test=True)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["license_discovery"]
