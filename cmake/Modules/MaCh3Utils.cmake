#KS: Inspired by Nuisance code
if(NOT COMMAND DefineEnabledRequiredSwitch)
  function(DefineEnabledRequiredSwitch VARNAME DEFAULTVALUE)
    if(NOT DEFINED ${VARNAME} OR "${${VARNAME}}x" STREQUAL "x")
      SET(${VARNAME} ${DEFAULTVALUE} PARENT_SCOPE)
      SET(${VARNAME} ${DEFAULTVALUE})
    else()
      SET(${VARNAME}_REQUIRED ${${VARNAME}} PARENT_SCOPE)
      SET(${VARNAME}_REQUIRED ${${VARNAME}})
    endif()
  endfunction()
endif()

#KS: Some cmake use 0 or 1 instead of true/false, weird I know. This simple tool will translate it for us
if(NOT COMMAND IsTrue)
  function(IsTrue VAR RESULT)
    if(NOT DEFINED ${VAR} OR NOT ${${VAR}})
      set(${RESULT} 0 PARENT_SCOPE)
    else()
      set(${RESULT} 1 PARENT_SCOPE)
    endif()
  endfunction()
endif()
