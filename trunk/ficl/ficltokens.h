/*
** Copyright (c) 1997-2001 John Sadler (john_sadler@alum.mit.edu)
** All rights reserved.
**
** Get the latest Ficl release at http://ficl.sourceforge.net
**
** I am interested in hearing from anyone who uses Ficl. If you have
** a problem, a success story, a defect, an enhancement request, or
** if you would like to contribute to the Ficl release, please
** contact me by email at the address above.
**
** L I C E N S E  and  D I S C L A I M E R
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

/*-
 * Adapted to work with FTH
 *
 * Copyright (c) 2004-2013 Michael Scholz <mi-scholz@users.sourceforge.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)ficltokens.h	1.24 10/17/13
 */

FICL_TOKEN(ficlInstructionInvalid, "** invalid **")
FICL_TOKEN(ficlInstruction1, 	     "1")
FICL_TOKEN(ficlInstruction2, 	     "2")
FICL_TOKEN(ficlInstruction3, 	     "3")
FICL_TOKEN(ficlInstruction4, 	     "4")
FICL_TOKEN(ficlInstruction5, 	     "5")
FICL_TOKEN(ficlInstruction6, 	     "6")
FICL_TOKEN(ficlInstruction7, 	     "7")
FICL_TOKEN(ficlInstruction8, 	     "8")
FICL_TOKEN(ficlInstruction9, 	     "9")
FICL_TOKEN(ficlInstruction10, 	    "10")
FICL_TOKEN(ficlInstruction11, 	    "11")
FICL_TOKEN(ficlInstruction12, 	    "12")
FICL_TOKEN(ficlInstruction13, 	    "13")
FICL_TOKEN(ficlInstruction14, 	    "14")
FICL_TOKEN(ficlInstruction15, 	    "15")
FICL_TOKEN(ficlInstruction16, 	    "16")
FICL_TOKEN(ficlInstruction0,         "0")
FICL_TOKEN(ficlInstructionNeg1,     "-1")
FICL_TOKEN(ficlInstructionNeg2,     "-2")
FICL_TOKEN(ficlInstructionNeg3,     "-3")
FICL_TOKEN(ficlInstructionNeg4,     "-4")
FICL_TOKEN(ficlInstructionNeg5,     "-5")
FICL_TOKEN(ficlInstructionNeg6,     "-6")
FICL_TOKEN(ficlInstructionNeg7,     "-7")
FICL_TOKEN(ficlInstructionNeg8,     "-8")
FICL_TOKEN(ficlInstructionNeg9,     "-9")
FICL_TOKEN(ficlInstructionNeg10,   "-10")
FICL_TOKEN(ficlInstructionNeg11,   "-11")
FICL_TOKEN(ficlInstructionNeg12,   "-12")
FICL_TOKEN(ficlInstructionNeg13,   "-13")
FICL_TOKEN(ficlInstructionNeg14,   "-14")
FICL_TOKEN(ficlInstructionNeg15,   "-15")
FICL_TOKEN(ficlInstructionNeg16,   "-16")
FICL_TOKEN(ficlInstructionF0,      "0.0")
FICL_TOKEN(ficlInstructionF1,      "1.0")
FICL_TOKEN(ficlInstructionFNeg1,  "-1.0")
FICL_INSTRUCTION_TOKEN(ficlInstructionSemiParen,       "(;)",               FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionExitParen,       "(exit)",            FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionDup,             "dup",               FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionSwap,            "swap",              FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionBranchParenWithCheck, "(branch)",     FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionBranchParen,     "(branch-final)",    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionBranch0ParenWithCheck, "(branch0)",   FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionBranch0Paren,    "(branch0-final)",   FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionLiteralParen,    "(literal)",         FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionLoopParen,       "(loop)",            FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionOfParen,         "(of)",              FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionPlusLoopParen,   "(+loop)",           FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionFetch,           "@",                 FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionStore,           "!",                 FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionComma,           ",",                 FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionCompileComma,    "compile,",          FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionCComma,          "c,",                FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionCells,           "cells",             FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionCellPlus,        "cell+",             FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionStarSlash,       "*/",                FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionSlashMod,        "/mod",              FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionStarSlashMod,    "*/mod",             FICL_WORD_DEFAULT)

FICL_INSTRUCTION_TOKEN(ficlInstructionColonParen,      "** (colon) **",     FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionVariableParen,   "(variable)",        FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionConstantParen,   "(constant)",        FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionDoDoes,          "** do-does **",     FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionDoParen,         "(do)",              FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionDoesParen,       "(does)",            FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionQDoParen,        "(?do)",             FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionCreateParen,     "(create)",          FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionStringLiteralParen,  "(.\")",         FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionCStringLiteralParen, "(c\")",         FICL_WORD_COMPILE_ONLY)

FICL_INSTRUCTION_TOKEN(ficlInstructionPlusStore,       "+!",        	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionOver,            "over",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionRot,             "rot",       	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction2Drop,           "2drop",     	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction2Dup,            "2dup",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction2Over,           "2over",     	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction2Swap,           "2swap",     	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFromRStack,      "r>",        	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionFetchRStack,     "r@",        	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstruction2ToR,            "2>r",       	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstruction2RFrom,          "2r>",       	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstruction2RFetch,         "2r@",       	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionToRStack,        ">r",        	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionQuestionDup,     "?dup",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionAnd,             "and",       	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionCStore,          "c!",        	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionCFetch,          "c@",        	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionDrop,            "drop",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionPick,            "pick",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionNip,             "nip",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionTuck,            "tuck",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionRoll,            "roll",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionMinusRoll,       "-roll",     	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionMinusRot,        "-rot",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFill,            "fill",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionWithin,          "within",       	    FICL_WORD_DEFAULT)

FICL_INSTRUCTION_TOKEN(ficlInstructionInvert,          "invert",    	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionLShift,          "lshift",    	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionMax,             "max",       	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionMin,             "min",       	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionMove,            "move",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionOr,              "or",        	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionRShift,          "rshift",    	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionXor,             "xor",       	    FICL_WORD_DEFAULT)

FICL_INSTRUCTION_TOKEN(ficlInstructionI,               "i",         	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionJ,               "j",         	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionK,               "k",         	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionCompare,         "compare",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionCompareInsensitive, "compare-insensitive", FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionLeave,           "leave",             FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionUnloop,          "unloop",            FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionUserParen,       "(user)", 	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionLinkParen,       "(link)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionUnlinkParen,     "(unlink)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionGetLocalParen,   "(@local)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionToLocalParen,    "(toLocal)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionGetLocal0,       "(@local0)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionToLocal0,        "(toLocal0)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionGetLocal1,       "(@local1)",  	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionToLocal1,        "(toLocal1)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionPlusToLocalParen,"(+toLocal)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionPlusToLocal0,    "(+toLocal0)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionPlusToLocal1,    "(+toLocal1)", 	    FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionFPlusToLocalParen,"(f+toLocal)",      FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionFPlusToLocal0,    "(f+toLocal0)",     FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficlInstructionFPlusToLocal1,    "(f+toLocal1)",     FICL_WORD_COMPILE_ONLY)

FICL_INSTRUCTION_TOKEN(ficlInstruction0Equals,         "0=",        	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_zero_p,          	       "zero?",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_0_not_eql,       	       "0<>",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction0Less,           "0<",        	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_negative_p,      	       "negative?",  	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction0Greater,        "0>",        	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_0_less_eql,      	       "0<=",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_0_greater_eql,   	       "0>=",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_positive_p,      	       "positive?",  	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionEquals,          "=",         	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_not_eql,         	       "<>",    	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionLess,            "<",         	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionGreaterThan,     ">",                 FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_less_eql,        	       "<=",    	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_greater_eql,     	       ">=",    	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionPlus,            "+",                 FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionMinus,           "-",                 FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionStar,            "*",                 FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionSlash,           "/",                 FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction1Plus,           "1+",                FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction1Minus,          "1-",                FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction2Plus,           "2+",                FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction2Minus,          "2-",                FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction2Star,           "2*",                FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstruction2Slash,          "2/",                FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionNegate,          "negate",            FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_abs,   	               "abs",   	    FICL_WORD_DEFAULT)

FICL_INSTRUCTION_TOKEN(ficlInstructionF0Equals,        "f0=", 		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f_zero_p,        	       "fzero?",     	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f0_not_eql,      	       "f0<>",  	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionF0Less,          "f0<", 		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f_negative_p,    	       "fnegative?", 	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionF0Greater,       "f0>", 		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f0_less_eql,     	       "f0<=",  	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f0_greater_eql,  	       "f0>=",  	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f_positive_p,    	       "fpositive?", 	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFEquals,         "f=",  		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f_not_eql,       	       "f<>",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFLess,           "f<",  		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFGreater,        "f>",  		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f_less_eql,      	       "f<=",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f_greater_eql,   	       "f>=",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFPlus,  	       "f+", 		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFMinus, 	       "f-", 		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFStar,  	       "f*", 		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFSlash, 	       "f/", 		    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f2_star,         	       "f2*",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f2_slash,        	       "f2/",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_1_slash_f,       	       "1/f",   	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFNegate,	       "fnegate",           FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFPlusStore,      "f+!",    	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f_max,           	       "fmax",       	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_f_min,           	       "fmin",       	    FICL_WORD_DEFAULT)

/* procs */
FICL_INSTRUCTION_TOKEN(ficl_execute_func,    	       "(execute-func)",    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_execute_void_func,         "(execute-void-func)", FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_execute_trace_var,         "(trace-var-execute)", FICL_WORD_COMPILE_ONLY)
FICL_INSTRUCTION_TOKEN(ficl_word_location,    	       "(word-location)",   FICL_WORD_DEFAULT)
/* bool */
FICL_INSTRUCTION_TOKEN(ficl_bool_false,      	       "false",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_bool_true,       	       "true",       	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_bool_and,        	       "&&",         	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_bool_or,         	       "||",         	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficl_bool_not,        	       "not",        	    FICL_WORD_DEFAULT)
/* float */
FICL_INSTRUCTION_TOKEN(ficlInstructionFOver,           "fover",      	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFDrop,           "fdrop",     	    FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFDup,            "fdup",              FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFSwap,           "fswap",             FICL_WORD_DEFAULT)
FICL_INSTRUCTION_TOKEN(ficlInstructionFRot,            "frot",       	    FICL_WORD_DEFAULT)

FICL_TOKEN(ficlInstructionExitInnerLoop, "** exit inner loop **")
