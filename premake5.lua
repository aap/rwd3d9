workspace "rwd3d9"
   configurations { "Release", "Debug" }
   location "build"
   
   files { "source/*.*" }

   includedirs { "source" }

project "rwd3d9"
   kind "SharedLib"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   targetextension ".dll"
   characterset ("MBCS")

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      targetdir "libs"
      defines { "NDEBUG" }
      optimize "On"
	  flags { "StaticRuntime" }
	  