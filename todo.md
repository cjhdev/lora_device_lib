Todo
====

- Implement the LoRaWAN conformance test application
    - The test app should be "built-in" and run from LDL_MAC_process()
    - The test app should be included in the build if LDL_ENABLE_TEST_MODE is defined
    - The test app should be started and stopped from a LDL_MAC_* interface

- LoRaWAN conformance test
    - need to put LDL through the test

- Class B
    - plan for how this feature can integrate with existing patterns
    - implement mode
    - implement tooling to prove the feature works

- Class C
    - plan for how this feature can integrate with existing patterns
    - implement mode
    - implement tooling to prove the feature works

- Add support for additional regions
    - CN_779
    - CN_470_510
    - AS_923
    - KR_920_923
    - IN_865_867
    - RU_864_870

- Radio driver improvements
    - add adjustment for current trim
    - add adjustment for PA ramp time
    - add FSK mode
    - add continuous wave mode
