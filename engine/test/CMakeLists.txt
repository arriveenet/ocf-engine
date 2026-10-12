# ==================================================================================================
# Tests
# ==================================================================================================
add_executable(test_${TARGET}
    test_allocator.cpp
    test_apu.cpp
    test_audio_mixer.cpp
    test_audio_tracker_source.cpp
    test_event_dispatcher.cpp
    test_geometric.cpp
    test_imgui.cpp
    test_jobsystem.cpp
    test_main.cpp
    test_mat2.cpp
    test_mat3.cpp
    test_mat4.cpp
    test_matrix_transform.cpp
    test_quat.cpp
    test_rect.cpp
    test_reference.cpp
    test_result.cpp
    test_scene_manager.cpp
    test_vec.cpp
    test_texture_sampler.cpp
    test_text_layout.cpp
    test_text_utility.cpp
    test_song_player.cpp
    test_tracker_notation.cpp
    test_tracker_song.cpp
)
target_link_libraries(test_${TARGET} PRIVATE gtest ocfengine)

# For tests of internal classes (e.g. audio/tracker/SongPlayer.h)
target_include_directories(test_${TARGET} PRIVATE ${ocfengine_SOURCE_DIR}/src)

set_target_properties(test_${TARGET} PROPERTIES FOLDER Tests)

# ==================================================================================================
#  Compiler flags
# ==================================================================================================
if (MSVC)
    target_compile_options(test_${TARGET} PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    target_compile_options(test_${TARGET} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()
