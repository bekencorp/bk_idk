if (CONFIG_PHY_TEST)
	list(APPEND incs phy)

	list(APPEND srcs phy/phy_test.c)

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
