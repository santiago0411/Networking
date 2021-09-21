workspace "Networking"
	architecture "x86_64"
	startproject "NetClient"

	configurations
	{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Networking"
include "NetClient"
include "NetServer"