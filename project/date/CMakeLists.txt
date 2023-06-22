set(PACKAGE_NAME date)

# Library target
add_library(date INTERFACE)
target_include_directories(date
  INTERFACE
  ${PROJECT_SOURCE_DIR}
)