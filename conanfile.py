import os
import re

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, load


class SockppConan(ConanFile):
    name = "sockpp"
    description = "Modern C++ socket library, wrapping the BSD/POSIX/Winsock socket API."
    license = "BSD-3-Clause"
    author = "Frank Pagliughi"
    url = "https://github.com/fpagliughi/sockpp"
    homepage = "https://github.com/fpagliughi/sockpp"
    topics = ("sockets", "tcp", "udp", "unix", "network", "tls", "ssl")

    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_openssl": [True, False],
        "with_mbedtls": [True, False],
        "unix_sockets": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_openssl": False,
        "with_mbedtls": False,
        "unix_sockets": True,
    }

    exports_sources = "CMakeLists.txt", "cmake/*", "src/*", "include/*", "LICENSE"

    def set_version(self):
        cmakelists = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
        match = re.search(r'project\(\s*sockpp\s+VERSION\s+"?([\d.]+)"?', cmakelists)
        if not match:
            raise ConanInvalidConfiguration(
                "Could not determine the sockpp version from CMakeLists.txt"
            )
        self.version = match.group(1)

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def validate(self):
        check_min_cppstd(self, 17)
        if self.options.with_openssl and self.options.with_mbedtls:
            raise ConanInvalidConfiguration(
                "sockpp: 'with_openssl' and 'with_mbedtls' are mutually exclusive"
            )

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        if self.options.with_openssl:
            self.requires("openssl/[>=3.0 <4]")
        elif self.options.with_mbedtls:
            self.requires("mbedtls/[>=3.5 <4]")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["SOCKPP_BUILD_SHARED"] = bool(self.options.shared)
        tc.variables["SOCKPP_BUILD_STATIC"] = not bool(self.options.shared)
        tc.variables["SOCKPP_BUILD_EXAMPLES"] = False
        tc.variables["SOCKPP_BUILD_TESTS"] = False
        tc.variables["SOCKPP_BUILD_DOCUMENTATION"] = False
        tc.variables["SOCKPP_WITH_OPENSSL"] = bool(self.options.with_openssl)
        tc.variables["SOCKPP_WITH_MBEDTLS"] = bool(self.options.with_mbedtls)
        if self.settings.os != "Windows":
            tc.variables["SOCKPP_WITH_UNIX_SOCKETS"] = bool(self.options.unix_sockets)
        fpic = self.options.get_safe("fPIC")
        if fpic is not None:
            tc.variables["CMAKE_POSITION_INDEPENDENT_CODE"] = bool(fpic)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(
            self,
            "LICENSE",
            src=self.source_folder,
            dst=os.path.join(self.package_folder, "licenses"),
        )
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "sockpp")
        self.cpp_info.set_property("cmake_target_name", "Sockpp::sockpp")
        self.cpp_info.libs = ["sockpp"]

        if self.settings.os == "Windows":
            self.cpp_info.system_libs = ["ws2_32"]
