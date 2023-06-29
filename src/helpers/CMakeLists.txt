add_executable(TestSmartBuffer buffer/TestSmartBuffer.cpp)
add_test(NAME TestSmartBuffer COMMAND TestSmartBuffer)

#[[
set (passRegex "^Test passed" "^All ok")
set (failRegex "Error" "Fail")

set_property (TEST outputTest
        PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
set_property (TEST outputTest
        PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")]]
