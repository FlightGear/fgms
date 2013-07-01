/** @file debug_types.h
 *  @brief Define the various logging classes and prioritiess
 */

/** 
 * @brief Define the possible classes/categories of logging messages
 */
typedef enum {
	SG_NONE		= 0x00000000,
	SG_FGMS		= 0x00000001,
	SG_FGTRACKER	= 0x00000002,
	SG_UTIL		= 0x00001000,	// utility functions
	SG_CONSOLE	= 0x00002000,	// log to console
	SG_SYSTEMS	= 0x00040000,
	SG_UNDEFD	= 0x00080000,	// For range checking

	SG_ALL		= 0xFFFFFFFF
} sgDebugClass;


/**
 * @brief Define the possible logging priorities (and their order).
 */
typedef enum {
	
	SG_DISABLED = 0,
	/** @brief For frequent messages */
	SG_BULK,

	/** @brief Less frequent debug type messages */
	SG_DEBUG,

	/** @brief Informatory messages */
	SG_INFO,

	/** @brief Possible impending problem */
	SG_WARN, 

	/** @brief  Very possible impending problem */
	SG_ALERT,           // 
	// SG_EXIT,        // Problem (no core)
	// SG_ABORT        // Abandon ship (core)
} sgDebugPriority;

