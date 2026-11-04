cmake_minimum_required(VERSION 3.20)
project(godfield_rl VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(GF_SANITIZE "Enable address/undefined sanitizers" OFF)
add_library(godfield SHARED src/env.cpp src/c_api.cpp src/catalog.cpp src/miracles.cpp src/elements.cpp src/chance.cpp src/reflection.cpp src/disease.cpp src/harms.cpp src/celestial.cpp src/offensive_miracles.cpp src/defensive_miracles.cpp src/attribute_change.cpp src/attribute_removal.cpp src/counter.cpp src/counter_extensions.cpp src/money_counter.cpp src/magic_reflection.cpp src/any_reflection.cpp src/mana_weapon.cpp src/magic_block.cpp src/additive_miracles.cpp src/doubling_miracles.cpp src/wide_miracles.cpp src/magic_free.cpp src/forgetting.cpp src/exchange.cpp src/revival.cpp src/item_removal.cpp src/hp_absorption.cpp src/self_damage.cpp src/pestle_mortar.cpp src/selling.cpp src/buying.cpp src/hp_gamble.cpp src/magic_bounce.cpp src/sundry_boost.cpp src/status_miracles.cpp src/dying_weapon.cpp
  src/legacy_env.cpp src/legacy_c_api.cpp src/miracles_c_api.cpp src/elements_c_api.cpp src/chance_c_api.cpp src/reflection_c_api.cpp src/disease_c_api.cpp src/harms_c_api.cpp src/celestial_c_api.cpp src/offensive_miracles_c_api.cpp src/defensive_miracles_c_api.cpp src/attribute_change_c_api.cpp src/attribute_removal_c_api.cpp src/counter_c_api.cpp src/counter_extensions_c_api.cpp src/money_counter_c_api.cpp src/magic_reflection_c_api.cpp src/any_reflection_c_api.cpp src/mana_weapon_c_api.cpp src/magic_block_c_api.cpp src/additive_miracles_c_api.cpp src/doubling_miracles_c_api.cpp src/wide_miracles_c_api.cpp src/magic_free_c_api.cpp src/forgetting_c_api.cpp src/exchange_c_api.cpp src/revival_c_api.cpp src/item_removal_c_api.cpp src/hp_absorption_c_api.cpp src/self_damage_c_api.cpp src/pestle_mortar_c_api.cpp src/selling_c_api.cpp src/buying_c_api.cpp src/hp_gamble_c_api.cpp src/magic_bounce_c_api.cpp src/sundry_boost_c_api.cpp src/status_miracles_c_api.cpp src/dying_weapon_c_api.cpp)
target_include_directories(godfield PUBLIC include)
target_sources(godfield PRIVATE src/official_catalog.cpp src/official_rules.cpp src/official_c_api.cpp
  src/official_state.cpp src/official_state_c_api.cpp src/official_display.cpp src/official_display_c_api.cpp src/official_match.cpp src/official_match_c_api.cpp)
target_compile_options(godfield PRIVATE -Wall -Wextra -Wpedantic)
if(GF_SANITIZE)
  target_compile_options(godfield PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(godfield PUBLIC -fsanitize=address,undefined)
endif()
add_executable(godfield_bench src/bench.cpp)
find_package(Threads REQUIRED)
target_link_libraries(godfield_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_legacy_bench src/legacy_bench.cpp)
target_link_libraries(godfield_legacy_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_official_bench src/official_match_bench.cpp)
target_link_libraries(godfield_official_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_guardian_bench src/official_guardian_bench.cpp)
target_link_libraries(godfield_guardian_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_guardian_payload_bench src/official_guardian_payload_bench.cpp)
target_link_libraries(godfield_guardian_payload_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_phenomenon_bench src/official_phenomenon_bench.cpp)
target_link_libraries(godfield_phenomenon_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_targeted_phenomenon_bench src/official_targeted_phenomenon_bench.cpp)
target_link_libraries(godfield_targeted_phenomenon_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_item_shuffle_bench src/official_item_shuffle_bench.cpp)
target_link_libraries(godfield_item_shuffle_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_confusion_bench src/official_confusion_bench.cpp)
target_link_libraries(godfield_confusion_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_fate_bench src/official_fate_bench.cpp)
target_link_libraries(godfield_fate_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_devil_bench src/official_devil_bench.cpp)
target_link_libraries(godfield_devil_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_sacrifice_bench src/official_sacrifice_bench.cpp)
target_link_libraries(godfield_sacrifice_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_grant_bench src/official_grant_bench.cpp)
target_link_libraries(godfield_grant_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_grant_sampler_bench src/official_grant_sampler_bench.cpp)
target_link_libraries(godfield_grant_sampler_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_fate_pool_bench src/official_fate_pool_bench.cpp)
target_link_libraries(godfield_fate_pool_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_fate_sampler_bench src/official_fate_sampler_bench.cpp)
target_link_libraries(godfield_fate_sampler_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_confusion_sampler_bench src/official_confusion_sampler_bench.cpp)
target_link_libraries(godfield_confusion_sampler_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_guardian_payload_sampler_bench src/official_guardian_payload_sampler_bench.cpp)
target_link_libraries(godfield_guardian_payload_sampler_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_guardian_payload_auto_bench src/official_guardian_payload_auto_bench.cpp)
target_link_libraries(godfield_guardian_payload_auto_bench PRIVATE godfield Threads::Threads)
add_executable(godfield_driver_bench src/official_driver_bench.cpp)
target_link_libraries(godfield_driver_bench PRIVATE godfield Threads::Threads)
include(CTest)
if(BUILD_TESTING)
  add_executable(godfield_official_live_trace_tests tests/official_live_trace_test.cpp)
  target_link_libraries(godfield_official_live_trace_tests PRIVATE godfield)
  add_test(NAME official_live_trace COMMAND godfield_official_live_trace_tests)
  add_executable(godfield_official_active_display_tests tests/official_active_display_test.cpp)
  target_link_libraries(godfield_official_active_display_tests PRIVATE godfield)
  add_test(NAME official_active_display COMMAND godfield_official_active_display_tests)
  add_executable(godfield_official_active_display_vectors_tests tests/official_active_display_vectors_test.cpp)
  target_link_libraries(godfield_official_active_display_vectors_tests PRIVATE godfield)
  add_test(NAME official_active_display_vectors COMMAND godfield_official_active_display_vectors_tests ${CMAKE_CURRENT_SOURCE_DIR}/data/official-v2/active-display-vectors-v50.tsv)
  add_executable(godfield_official_display_api_tests tests/official_display_c_api_test.cpp)
  target_link_libraries(godfield_official_display_api_tests PRIVATE godfield)
  add_test(NAME official_display_c_api COMMAND godfield_official_display_api_tests)
  add_executable(godfield_official_driver_tests tests/official_driver_test.cpp)
  target_link_libraries(godfield_official_driver_tests PRIVATE godfield)
  add_test(NAME official_driver COMMAND godfield_official_driver_tests)
  add_test(NAME official_driver_benchmark COMMAND godfield_driver_bench 600 9 42 2)
  add_executable(godfield_official_active_attack_tests tests/official_active_attack_test.cpp)
  target_link_libraries(godfield_official_active_attack_tests PRIVATE godfield)
  add_test(NAME official_active_attack COMMAND godfield_official_active_attack_tests)
  add_executable(godfield_official_active_attack_vectors_tests tests/official_active_attack_vectors_test.cpp)
  target_link_libraries(godfield_official_active_attack_vectors_tests PRIVATE godfield)
  add_test(NAME official_active_attack_vectors COMMAND godfield_official_active_attack_vectors_tests ${CMAKE_CURRENT_SOURCE_DIR}/data/official-v2/active-attack-vectors-v48.tsv)
endif()
if(BUILD_TESTING)
  add_executable(godfield_official_catalog_tests tests/official_catalog_test.cpp)
  target_link_libraries(godfield_official_catalog_tests PRIVATE godfield)
  add_test(NAME official_catalog COMMAND godfield_official_catalog_tests)
  add_executable(godfield_official_rules_tests tests/official_rules_test.cpp)
  target_link_libraries(godfield_official_rules_tests PRIVATE godfield)
  add_test(NAME official_client_rules COMMAND godfield_official_rules_tests)
  add_executable(godfield_official_vectors_tests tests/official_client_vectors_test.cpp)
  target_link_libraries(godfield_official_vectors_tests PRIVATE godfield)
  add_test(NAME official_client_vectors COMMAND godfield_official_vectors_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/data/official-v2/client-vectors.tsv)
  add_executable(godfield_official_api_tests tests/official_c_api_test.cpp)
  target_link_libraries(godfield_official_api_tests PRIVATE godfield)
  add_test(NAME official_c_api COMMAND godfield_official_api_tests)
  add_executable(godfield_official_state_tests tests/official_state_test.cpp)
  target_link_libraries(godfield_official_state_tests PRIVATE godfield)
  add_test(NAME official_state COMMAND godfield_official_state_tests)
  add_executable(godfield_official_state_vectors_tests tests/official_state_vectors_test.cpp)
  target_link_libraries(godfield_official_state_vectors_tests PRIVATE godfield)
  add_test(NAME official_state_vectors COMMAND godfield_official_state_vectors_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/data/official-v2/state-vectors.tsv)
  add_executable(godfield_official_state_api_tests tests/official_state_c_api_test.cpp)
  target_link_libraries(godfield_official_state_api_tests PRIVATE godfield)
  add_test(NAME official_state_c_api COMMAND godfield_official_state_api_tests)
  add_executable(godfield_official_match_tests tests/official_match_test.cpp)
  target_link_libraries(godfield_official_match_tests PRIVATE godfield)
  # Test cases have internal linkage: a missing main() registration must fail the build.
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(godfield_official_match_tests PRIVATE -Wunused-function -Werror=unused-function)
  endif()
  add_test(NAME official_match COMMAND godfield_official_match_tests)
  add_test(NAME official_guardian_benchmark COMMAND godfield_guardian_bench 420 9 42 2)
  add_test(NAME official_guardian_payload_benchmark COMMAND godfield_guardian_payload_bench 456 9 42 2)
  add_test(NAME official_phenomenon_benchmark COMMAND godfield_phenomenon_bench 500 9 42 2)
  add_test(NAME official_targeted_phenomenon_benchmark COMMAND godfield_targeted_phenomenon_bench 600 9 42 2)
  add_test(NAME official_item_shuffle_benchmark COMMAND godfield_item_shuffle_bench 600 9 42 2)
  add_test(NAME official_confusion_benchmark COMMAND godfield_confusion_bench 600 9 42 2)
  add_test(NAME official_fate_benchmark COMMAND godfield_fate_bench 600 9 42 2)
  add_test(NAME official_devil_benchmark COMMAND godfield_devil_bench 600 9 42 2)
  add_test(NAME official_sacrifice_benchmark COMMAND godfield_sacrifice_bench 600 9 42 2)
  add_test(NAME official_grant_benchmark COMMAND godfield_grant_bench 600 9 42 2)
  add_test(NAME official_grant_sampler_benchmark COMMAND godfield_grant_sampler_bench 600 9 42 2)
  add_test(NAME official_fate_pool_benchmark COMMAND godfield_fate_pool_bench 600 9 42 2)
  add_test(NAME official_fate_sampler_benchmark COMMAND godfield_fate_sampler_bench 600 9 42 2)
  add_test(NAME official_confusion_sampler_benchmark COMMAND godfield_confusion_sampler_bench 600 9 42 2)
  add_test(NAME official_guardian_payload_sampler_benchmark COMMAND godfield_guardian_payload_sampler_bench 600 9 42 2)
  add_test(NAME official_guardian_payload_auto_benchmark COMMAND godfield_guardian_payload_auto_bench 600 9 42 2)
  add_executable(godfield_official_resource_tests tests/official_resource_vectors_test.cpp)
  target_link_libraries(godfield_official_resource_tests PRIVATE godfield)
  add_test(NAME official_resource_vectors COMMAND godfield_official_resource_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/data/official-v2/resource-vectors.tsv)
  add_executable(godfield_official_match_api_tests tests/official_match_c_api_test.cpp)
  target_link_libraries(godfield_official_match_api_tests PRIVATE godfield)
  add_test(NAME official_match_c_api COMMAND godfield_official_match_api_tests)
  add_executable(godfield_tests tests/env_test.cpp)
  target_link_libraries(godfield_tests PRIVATE godfield)
  add_test(NAME environment COMMAND godfield_tests)
  add_executable(godfield_legacy_tests tests/legacy_test.cpp)
  target_link_libraries(godfield_legacy_tests PRIVATE godfield)
  add_test(NAME legacy_environment COMMAND godfield_legacy_tests)
  add_executable(godfield_miracles_tests tests/miracles_test.cpp)
  target_link_libraries(godfield_miracles_tests PRIVATE godfield)
  add_test(NAME miracles_environment COMMAND godfield_miracles_tests)
  add_executable(godfield_elements_tests tests/elements_test.cpp)
  target_link_libraries(godfield_elements_tests PRIVATE godfield)
  add_test(NAME elements_environment COMMAND godfield_elements_tests)
  add_executable(godfield_chance_tests tests/chance_test.cpp)
  target_link_libraries(godfield_chance_tests PRIVATE godfield)
  add_test(NAME chance_environment COMMAND godfield_chance_tests)
  add_executable(godfield_reflection_tests tests/reflection_test.cpp)
  target_link_libraries(godfield_reflection_tests PRIVATE godfield)
  add_test(NAME reflection_environment COMMAND godfield_reflection_tests)
  add_executable(godfield_disease_tests tests/disease_test.cpp)
  target_link_libraries(godfield_disease_tests PRIVATE godfield)
  add_test(NAME disease_environment COMMAND godfield_disease_tests)
  add_executable(godfield_harms_tests tests/harms_test.cpp)
  target_link_libraries(godfield_harms_tests PRIVATE godfield)
  add_test(NAME harms_environment COMMAND godfield_harms_tests)
  add_executable(godfield_celestial_tests tests/celestial_test.cpp)
  target_link_libraries(godfield_celestial_tests PRIVATE godfield)
  add_test(NAME celestial_environment COMMAND godfield_celestial_tests)
  add_executable(godfield_offensive_miracles_tests tests/offensive_miracles_test.cpp)
  target_link_libraries(godfield_offensive_miracles_tests PRIVATE godfield)
  add_test(NAME offensive_miracles_environment COMMAND godfield_offensive_miracles_tests)
  add_executable(godfield_defensive_miracles_tests tests/defensive_miracles_test.cpp)
  target_link_libraries(godfield_defensive_miracles_tests PRIVATE godfield)
  add_test(NAME defensive_miracles_environment COMMAND godfield_defensive_miracles_tests)
  add_executable(godfield_attribute_change_tests tests/attribute_change_test.cpp)
  target_link_libraries(godfield_attribute_change_tests PRIVATE godfield)
  add_test(NAME attribute_change_environment COMMAND godfield_attribute_change_tests)
  add_executable(godfield_attribute_removal_tests tests/attribute_removal_test.cpp)
  target_link_libraries(godfield_attribute_removal_tests PRIVATE godfield)
  add_test(NAME attribute_removal_environment COMMAND godfield_attribute_removal_tests)
  add_executable(godfield_counter_tests tests/counter_test.cpp)
  target_link_libraries(godfield_counter_tests PRIVATE godfield)
  add_test(NAME counter_environment COMMAND godfield_counter_tests)
  add_executable(godfield_counter_extensions_tests tests/counter_extensions_test.cpp)
  target_link_libraries(godfield_counter_extensions_tests PRIVATE godfield)
  add_test(NAME counter_extensions_environment COMMAND godfield_counter_extensions_tests)
  add_executable(godfield_money_counter_tests tests/money_counter_test.cpp)
  target_link_libraries(godfield_money_counter_tests PRIVATE godfield)
  add_test(NAME money_counter_environment COMMAND godfield_money_counter_tests)
  add_executable(godfield_magic_reflection_tests tests/magic_reflection_test.cpp)
  target_link_libraries(godfield_magic_reflection_tests PRIVATE godfield)
  add_test(NAME magic_reflection_environment COMMAND godfield_magic_reflection_tests)
  add_executable(godfield_any_reflection_tests tests/any_reflection_test.cpp)
  target_link_libraries(godfield_any_reflection_tests PRIVATE godfield)
  add_test(NAME any_reflection_environment COMMAND godfield_any_reflection_tests)
  add_executable(godfield_mana_weapon_tests tests/mana_weapon_test.cpp)
  target_link_libraries(godfield_mana_weapon_tests PRIVATE godfield)
  add_test(NAME mana_weapon_environment COMMAND godfield_mana_weapon_tests)
  add_executable(godfield_magic_block_tests tests/magic_block_test.cpp)
  target_link_libraries(godfield_magic_block_tests PRIVATE godfield)
  add_test(NAME magic_block_environment COMMAND godfield_magic_block_tests)
  add_executable(godfield_additive_miracles_tests tests/additive_miracles_test.cpp)
  target_link_libraries(godfield_additive_miracles_tests PRIVATE godfield)
  add_test(NAME additive_miracles_environment COMMAND godfield_additive_miracles_tests)
  add_executable(godfield_doubling_miracles_tests tests/doubling_miracles_test.cpp)
  target_link_libraries(godfield_doubling_miracles_tests PRIVATE godfield)
  add_test(NAME doubling_miracles_environment COMMAND godfield_doubling_miracles_tests)
  add_executable(godfield_wide_miracles_tests tests/wide_miracles_test.cpp)
  target_link_libraries(godfield_wide_miracles_tests PRIVATE godfield)
  add_test(NAME wide_miracles_environment COMMAND godfield_wide_miracles_tests)
  # Base57 with initial MP5 and extra WIDE209 yields power0 after spell payment.
  add_test(NAME wide_miracles_benchmark COMMAND godfield_legacy_bench 10000 1 42 wide-miracles)
  add_executable(godfield_magic_free_tests tests/magic_free_test.cpp)
  target_link_libraries(godfield_magic_free_tests PRIVATE godfield)
  add_test(NAME magic_free_environment COMMAND godfield_magic_free_tests)
  add_executable(godfield_forgetting_tests tests/forgetting_test.cpp)
  target_link_libraries(godfield_forgetting_tests PRIVATE godfield)
  add_test(NAME forgetting_environment COMMAND godfield_forgetting_tests)
  add_executable(godfield_exchange_tests tests/exchange_test.cpp)
  target_link_libraries(godfield_exchange_tests PRIVATE godfield)
  add_test(NAME exchange_environment COMMAND godfield_exchange_tests)
  add_executable(godfield_revival_tests tests/revival_test.cpp)
  target_link_libraries(godfield_revival_tests PRIVATE godfield)
  add_test(NAME revival_environment COMMAND godfield_revival_tests)
  add_executable(godfield_item_removal_tests tests/item_removal_test.cpp)
  target_link_libraries(godfield_item_removal_tests PRIVATE godfield)
  add_test(NAME item_removal_environment COMMAND godfield_item_removal_tests)
  add_executable(godfield_hp_absorption_tests tests/hp_absorption_test.cpp)
  target_link_libraries(godfield_hp_absorption_tests PRIVATE godfield)
  add_test(NAME hp_absorption_environment COMMAND godfield_hp_absorption_tests)
  add_executable(godfield_self_damage_tests tests/self_damage_test.cpp)
  target_link_libraries(godfield_self_damage_tests PRIVATE godfield)
  add_test(NAME self_damage_environment COMMAND godfield_self_damage_tests)
  add_executable(godfield_pestle_mortar_tests tests/pestle_mortar_test.cpp)
  target_link_libraries(godfield_pestle_mortar_tests PRIVATE godfield)
  add_test(NAME pestle_mortar_environment COMMAND godfield_pestle_mortar_tests)
  add_executable(godfield_selling_tests tests/selling_test.cpp)
  target_link_libraries(godfield_selling_tests PRIVATE godfield)
  add_test(NAME selling_environment COMMAND godfield_selling_tests)
  add_executable(godfield_buying_tests tests/buying_test.cpp)
  target_link_libraries(godfield_buying_tests PRIVATE godfield)
  add_test(NAME buying_environment COMMAND godfield_buying_tests)
  add_executable(godfield_hp_gamble_tests tests/hp_gamble_test.cpp)
  target_link_libraries(godfield_hp_gamble_tests PRIVATE godfield)
  add_test(NAME hp_gamble_environment COMMAND godfield_hp_gamble_tests)
  add_executable(godfield_magic_bounce_tests tests/magic_bounce_test.cpp)
  target_link_libraries(godfield_magic_bounce_tests PRIVATE godfield)
  add_test(NAME magic_bounce_environment COMMAND godfield_magic_bounce_tests)
  add_executable(godfield_sundry_boost_tests tests/sundry_boost_test.cpp)
  target_link_libraries(godfield_sundry_boost_tests PRIVATE godfield)
  add_test(NAME sundry_boost_environment COMMAND godfield_sundry_boost_tests)
  add_executable(godfield_status_miracles_tests tests/status_miracles_test.cpp)
  target_link_libraries(godfield_status_miracles_tests PRIVATE godfield)
  add_test(NAME status_miracles_environment COMMAND godfield_status_miracles_tests)
  add_executable(godfield_dying_weapon_tests tests/dying_weapon_test.cpp)
  target_link_libraries(godfield_dying_weapon_tests PRIVATE godfield)
  add_test(NAME dying_weapon_environment COMMAND godfield_dying_weapon_tests)
  add_test(NAME sundry_boost_fresh_revival_benchmark COMMAND godfield_legacy_bench 1 1 78564 sundry-boost)
  set_tests_properties(sundry_boost_fresh_revival_benchmark PROPERTIES
    PASS_REGULAR_EXPRESSION "sale_revival_steps=1([^0-9]|$)")
  find_package(Python3 QUIET COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME official_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/official-v2/generate.py --check)
    add_test(NAME official_v1_archive_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_official_archive.py)
    add_test(NAME official_vectors_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_official_vectors.py)
    add_test(NAME catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-neutral-v1/generate.py --check)
    add_test(NAME miracles_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-miracles-v1/generate.py --check)
    add_test(NAME elements_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-elements-v1/generate.py --check)
    add_test(NAME chance_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-chance-v1/generate.py --check)
    add_test(NAME reflection_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-reflection-v1/generate.py --check)
    add_test(NAME disease_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-disease-v1/generate.py --check)
    add_test(NAME harms_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-harms-v1/generate.py --check)
    add_test(NAME celestial_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-celestial-v1/generate.py --check)
    add_test(NAME offensive_miracles_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-offensive-miracles-v1/generate.py --check)
    add_test(NAME defensive_miracles_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-defensive-miracles-v1/generate.py --check)
    add_test(NAME attribute_change_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-attribute-change-v1/generate.py --check)
    add_test(NAME attribute_removal_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-attribute-removal-v1/generate.py --check)
    add_test(NAME counter_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-counter-v1/generate.py --check)
    add_test(NAME counter_extensions_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-counter-extensions-v1/generate.py --check)
    add_test(NAME money_counter_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-money-counter-v1/generate.py --check)
    add_test(NAME magic_reflection_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-magic-reflection-v1/generate.py --check)
    add_test(NAME any_reflection_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-any-reflection-v1/generate.py --check)
    add_test(NAME mana_weapon_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-mana-weapon-v1/generate.py --check)
    add_test(NAME magic_block_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-magic-block-v1/generate.py --check)
    add_test(NAME additive_miracles_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-additive-miracles-v1/generate.py --check)
    add_test(NAME doubling_miracles_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-doubling-miracles-v1/generate.py --check)
    add_test(NAME wide_miracles_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-wide-miracles-v1/generate.py --check)
    add_test(NAME magic_free_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-magic-free-v1/generate.py --check)
    add_test(NAME forgetting_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-forgetting-v1/generate.py --check)
    add_test(NAME exchange_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-exchange-v1/generate.py --check)
    add_test(NAME revival_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-revival-v1/generate.py --check)
    add_test(NAME item_removal_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-item-removal-v1/generate.py --check)
    add_test(NAME status_miracles_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-status-miracles-v1/generate.py --check)
    add_test(NAME dying_weapon_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-dying-weapon-v1/generate.py --check)
    add_test(NAME sundry_boost_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-sundry-boost-v1/generate.py --check)
    add_test(NAME magic_bounce_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-magic-bounce-v1/generate.py --check)
    add_test(NAME hp_gamble_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-hp-gamble-v1/generate.py --check)
    add_test(NAME buying_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-buying-v1/generate.py --check)
    add_test(NAME selling_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-selling-v1/generate.py --check)
    add_test(NAME pestle_mortar_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-pestle-mortar-v1/generate.py --check)
    add_test(NAME self_damage_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-self-damage-v1/generate.py --check)
    add_test(NAME hp_absorption_catalog_integrity COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/data/legacy-hp-absorption-v1/generate.py --check)
    if(NOT GF_SANITIZE)
      add_test(NAME official_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/official_api_test.py $<TARGET_FILE:godfield>)
      add_test(NAME official_display_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/official_display_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME official_state_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/official_state_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME official_match_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/official_match_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME legacy_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/legacy_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME miracles_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/miracles_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME elements_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/elements_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME chance_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/chance_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME reflection_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/reflection_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME disease_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/disease_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME harms_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/harms_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME celestial_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/celestial_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME offensive_miracles_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/offensive_miracles_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME defensive_miracles_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/defensive_miracles_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME attribute_change_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/attribute_change_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME attribute_removal_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/attribute_removal_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME counter_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/counter_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME counter_extensions_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/counter_extensions_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME money_counter_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/money_counter_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME magic_reflection_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/magic_reflection_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME any_reflection_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/any_reflection_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME mana_weapon_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mana_weapon_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME magic_block_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/magic_block_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME additive_miracles_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/additive_miracles_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME doubling_miracles_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/doubling_miracles_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME wide_miracles_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/wide_miracles_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME magic_free_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/magic_free_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME forgetting_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/forgetting_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME exchange_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/exchange_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME revival_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/revival_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME item_removal_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/item_removal_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME status_miracles_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/status_miracles_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME dying_weapon_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/dying_weapon_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME sundry_boost_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/sundry_boost_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME magic_bounce_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/magic_bounce_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME hp_gamble_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/hp_gamble_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME buying_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/buying_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME selling_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/selling_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME pestle_mortar_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/pestle_mortar_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME self_damage_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/self_damage_python_test.py $<TARGET_FILE:godfield>)
      add_test(NAME hp_absorption_python COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/hp_absorption_python_test.py $<TARGET_FILE:godfield>)
    endif()
  endif()
endif()
