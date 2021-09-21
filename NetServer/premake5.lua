project "NetServer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Networking/include",
		"%{wks.location}/Networking/src",
		"%{wks.location}/Networking/vendor/asio",
	}

	filter "system:windows"
		systemversion "latest"
		defines "NET_WINDOWS"

	filter "configurations:Debug"
		defines "NET_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "NET_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "NET_DIST"
		runtime "Release"
		optimize "on"