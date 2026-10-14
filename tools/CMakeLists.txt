# ==================================================================================================
#  Sub Projects
# ==================================================================================================
# Command-line tools
add_subdirectory(dmcconv)

# The tracker is an ImGui application
if (OCF_ENABLE_IMGUI)
    add_subdirectory(tracker)
endif()
