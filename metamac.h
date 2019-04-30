// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once

//{{{ Delayed evaluation functors and other utilities ------------------

// Expands to nothing
#define PP_EMPTY(...)

// Evaluates to argument
#define PP_IDENTITY(...)	__VA_ARGS__

// Concatenates a and b
#define PP_CAT(a,b)		a##b
#define PP_CAT_I(a,b)		PP_CAT(a,b)

// Delays invocation of macro until after a is expanded
// Useful when a expands to a comma list of arguments
#define PP_EXPAND(m,a)		m(a)

// Turns argument into a string
#define PP_STRINGIFY(x)		#x
#define PP_STRINGIFY_I(x)	PP_STRINGIFY(x)

//}}}-------------------------------------------------------------------
//{{{ PP_REPEAT and other types of repetition

/// The maximum number of elements in PP_REPEAT, PP_LIST, and PP_ENUM
#define PP_MAX_REPEAT	32

//{{{2 PP_REPEAT - list with no separators. Repeats x N times.

#define PP_REPEAT_1(x)	x
#define PP_REPEAT_2(x)	PP_REPEAT_1(x) x
#define PP_REPEAT_3(x)	PP_REPEAT_2(x) x
#define PP_REPEAT_4(x)	PP_REPEAT_3(x) x
#define PP_REPEAT_5(x)	PP_REPEAT_4(x) x
#define PP_REPEAT_6(x)	PP_REPEAT_5(x) x
#define PP_REPEAT_7(x)	PP_REPEAT_6(x) x
#define PP_REPEAT_8(x)	PP_REPEAT_7(x) x
#define PP_REPEAT_9(x)	PP_REPEAT_8(x) x
#define PP_REPEAT_10(x)	PP_REPEAT_9(x) x
#define PP_REPEAT_11(x)	PP_REPEAT_10(x) x
#define PP_REPEAT_12(x)	PP_REPEAT_11(x) x
#define PP_REPEAT_13(x)	PP_REPEAT_12(x) x
#define PP_REPEAT_14(x)	PP_REPEAT_13(x) x
#define PP_REPEAT_15(x)	PP_REPEAT_14(x) x
#define PP_REPEAT_16(x)	PP_REPEAT_15(x) x
#define PP_REPEAT_17(x)	PP_REPEAT_16(x) x
#define PP_REPEAT_18(x)	PP_REPEAT_17(x) x
#define PP_REPEAT_19(x)	PP_REPEAT_18(x) x
#define PP_REPEAT_20(x)	PP_REPEAT_19(x) x
#define PP_REPEAT_21(x)	PP_REPEAT_20(x) x
#define PP_REPEAT_22(x)	PP_REPEAT_21(x) x
#define PP_REPEAT_23(x)	PP_REPEAT_22(x) x
#define PP_REPEAT_24(x)	PP_REPEAT_23(x) x
#define PP_REPEAT_25(x)	PP_REPEAT_24(x) x
#define PP_REPEAT_26(x)	PP_REPEAT_25(x) x
#define PP_REPEAT_27(x)	PP_REPEAT_26(x) x
#define PP_REPEAT_28(x)	PP_REPEAT_27(x) x
#define PP_REPEAT_29(x)	PP_REPEAT_28(x) x
#define PP_REPEAT_30(x)	PP_REPEAT_29(x) x
#define PP_REPEAT_31(x)	PP_REPEAT_30(x) x
#define PP_REPEAT_32(x)	PP_REPEAT_31(x) x
#define PP_REPEAT(N,x)	PP_CAT(PP_REPEAT_,N)(x)

//}}}2
//{{{2 PP_LIST - separated list. Repeats x N times with sep in between.

#define PP_LIST_1(x,d)	x(1)
#define PP_LIST_2(x,d)	PP_LIST_1(x,d) d x(2)
#define PP_LIST_3(x,d)	PP_LIST_2(x,d) d x(3)
#define PP_LIST_4(x,d)	PP_LIST_3(x,d) d x(4)
#define PP_LIST_5(x,d)	PP_LIST_4(x,d) d x(5)
#define PP_LIST_6(x,d)	PP_LIST_5(x,d) d x(6)
#define PP_LIST_7(x,d)	PP_LIST_6(x,d) d x(7)
#define PP_LIST_8(x,d)	PP_LIST_7(x,d) d x(8)
#define PP_LIST_9(x,d)	PP_LIST_8(x,d) d x(9)
#define PP_LIST_10(x,d)	PP_LIST_9(x,d) d x(10)
#define PP_LIST_11(x,d)	PP_LIST_10(x,d) d x(11)
#define PP_LIST_12(x,d)	PP_LIST_11(x,d) d x(12)
#define PP_LIST_13(x,d)	PP_LIST_12(x,d) d x(13)
#define PP_LIST_14(x,d)	PP_LIST_13(x,d) d x(14)
#define PP_LIST_15(x,d)	PP_LIST_14(x,d) d x(15)
#define PP_LIST_16(x,d)	PP_LIST_15(x,d) d x(16)
#define PP_LIST_17(x,d)	PP_LIST_16(x,d) d x(17)
#define PP_LIST_18(x,d)	PP_LIST_17(x,d) d x(18)
#define PP_LIST_19(x,d)	PP_LIST_18(x,d) d x(19)
#define PP_LIST_20(x,d)	PP_LIST_19(x,d) d x(20)
#define PP_LIST_21(x,d)	PP_LIST_20(x,d) d x(21)
#define PP_LIST_22(x,d)	PP_LIST_21(x,d) d x(22)
#define PP_LIST_23(x,d)	PP_LIST_22(x,d) d x(23)
#define PP_LIST_24(x,d)	PP_LIST_23(x,d) d x(24)
#define PP_LIST_25(x,d)	PP_LIST_24(x,d) d x(25)
#define PP_LIST_26(x,d)	PP_LIST_25(x,d) d x(26)
#define PP_LIST_27(x,d)	PP_LIST_26(x,d) d x(27)
#define PP_LIST_28(x,d)	PP_LIST_27(x,d) d x(28)
#define PP_LIST_29(x,d)	PP_LIST_28(x,d) d x(29)
#define PP_LIST_30(x,d)	PP_LIST_29(x,d) d x(30)
#define PP_LIST_31(x,d)	PP_LIST_30(x,d) d x(31)
#define PP_LIST_32(x,d)	PP_LIST_31(x,d) d x(32)
#define PP_LIST(N,x,d)	PP_CAT(PP_LIST_,N)(x,d)

//}}}2
//{{{2 PP_ENUM - Comma separated list. A special case of LIST needed because the preprocessor can't substitute commas.

#define PP_ENUM_1(x,d)	x(1,d)
#define PP_ENUM_2(x,d)	PP_ENUM_1(x,d), x(2,d)
#define PP_ENUM_3(x,d)	PP_ENUM_2(x,d), x(3,d)
#define PP_ENUM_4(x,d)	PP_ENUM_3(x,d), x(4,d)
#define PP_ENUM_5(x,d)	PP_ENUM_4(x,d), x(5,d)
#define PP_ENUM_6(x,d)	PP_ENUM_5(x,d), x(6,d)
#define PP_ENUM_7(x,d)	PP_ENUM_6(x,d), x(7,d)
#define PP_ENUM_8(x,d)	PP_ENUM_7(x,d), x(8,d)
#define PP_ENUM_9(x,d)	PP_ENUM_8(x,d), x(9,d)
#define PP_ENUM_10(x,d)	PP_ENUM_9(x,d), x(10,d)
#define PP_ENUM_11(x,d)	PP_ENUM_10(x,d), x(11,d)
#define PP_ENUM_12(x,d)	PP_ENUM_11(x,d), x(12,d)
#define PP_ENUM_13(x,d)	PP_ENUM_12(x,d), x(13,d)
#define PP_ENUM_14(x,d)	PP_ENUM_13(x,d), x(14,d)
#define PP_ENUM_15(x,d)	PP_ENUM_14(x,d), x(15,d)
#define PP_ENUM_16(x,d)	PP_ENUM_15(x,d), x(16,d)
#define PP_ENUM_17(x,d)	PP_ENUM_16(x,d), x(17,d)
#define PP_ENUM_18(x,d)	PP_ENUM_17(x,d), x(18,d)
#define PP_ENUM_19(x,d)	PP_ENUM_18(x,d), x(19,d)
#define PP_ENUM_20(x,d)	PP_ENUM_19(x,d), x(20,d)
#define PP_ENUM_21(x,d)	PP_ENUM_20(x,d), x(21,d)
#define PP_ENUM_22(x,d)	PP_ENUM_21(x,d), x(22,d)
#define PP_ENUM_23(x,d)	PP_ENUM_22(x,d), x(23,d)
#define PP_ENUM_24(x,d)	PP_ENUM_23(x,d), x(24,d)
#define PP_ENUM_25(x,d)	PP_ENUM_24(x,d), x(25,d)
#define PP_ENUM_26(x,d)	PP_ENUM_25(x,d), x(26,d)
#define PP_ENUM_27(x,d)	PP_ENUM_26(x,d), x(27,d)
#define PP_ENUM_28(x,d)	PP_ENUM_27(x,d), x(28,d)
#define PP_ENUM_29(x,d)	PP_ENUM_28(x,d), x(29,d)
#define PP_ENUM_30(x,d)	PP_ENUM_29(x,d), x(30,d)
#define PP_ENUM_31(x,d)	PP_ENUM_30(x,d), x(31,d)
#define PP_ENUM_32(x,d)	PP_ENUM_31(x,d), x(32,d)
#define PP_ENUM(N,x,d)	PP_CAT(PP_ENUM_,N)(x,d)

//}}}2
//}}}-------------------------------------------------------------------
//{{{ Sequences (a)(b)(c)

//{{{2 SEQ_SEP_N
#define SEQ_SEP_0(...)		(__VA_ARGS__),
#define SEQ_SEP_1(...)		SEQ_SEP_0
#define SEQ_SEP_2(...)		SEQ_SEP_1
#define SEQ_SEP_3(...)		SEQ_SEP_2
#define SEQ_SEP_4(...)		SEQ_SEP_3
#define SEQ_SEP_5(...)		SEQ_SEP_4
#define SEQ_SEP_6(...)		SEQ_SEP_5
#define SEQ_SEP_7(...)		SEQ_SEP_6
#define SEQ_SEP_8(...)		SEQ_SEP_7
#define SEQ_SEP_9(...)		SEQ_SEP_8
#define SEQ_SEP_10(...)		SEQ_SEP_9
#define SEQ_SEP_11(...)		SEQ_SEP_10
#define SEQ_SEP_12(...)		SEQ_SEP_11
#define SEQ_SEP_13(...)		SEQ_SEP_12
#define SEQ_SEP_14(...)		SEQ_SEP_13
#define SEQ_SEP_15(...)		SEQ_SEP_14
#define SEQ_SEP_16(...)		SEQ_SEP_15
#define SEQ_SEP_17(...)		SEQ_SEP_16
#define SEQ_SEP_18(...)		SEQ_SEP_17
#define SEQ_SEP_19(...)		SEQ_SEP_18
#define SEQ_SEP_20(...)		SEQ_SEP_19
#define SEQ_SEP_21(...)		SEQ_SEP_20
#define SEQ_SEP_22(...)		SEQ_SEP_21
#define SEQ_SEP_23(...)		SEQ_SEP_22
#define SEQ_SEP_24(...)		SEQ_SEP_23
#define SEQ_SEP_25(...)		SEQ_SEP_24
#define SEQ_SEP_26(...)		SEQ_SEP_25
#define SEQ_SEP_27(...)		SEQ_SEP_26
#define SEQ_SEP_28(...)		SEQ_SEP_27
#define SEQ_SEP_29(...)		SEQ_SEP_28
#define SEQ_SEP_30(...)		SEQ_SEP_29
#define SEQ_SEP_31(...)		SEQ_SEP_30
#define SEQ_SEP_32(...)		SEQ_SEP_31
//}}}2

#define SEQ_REMOVE_TAIL(x)	SEQ_REMOVE_TAIL_I(x)
#define SEQ_REMOVE_TAIL_I(x,_)	PP_IDENTITY x
#define SEQ_REMOVE_HEAD(x)	SEQ_REMOVE_HEAD_I(x)
#define SEQ_REMOVE_HEAD_I(_,x)	x

#define SEQ_SEP_BY_INDEX(seq,i)	SEQ_REMOVE_TAIL (SEQ_SEP_##i seq)
#define SEQ_HEAD(seq)		SEQ_SEP_BY_INDEX (seq, 0)
#define SEQ_POP_FRONT(seq)	SEQ_REMOVE_HEAD (SEQ_SEP_0 seq)

//{{{2 SEQ_SIZE_N
#define SEQ_SIZE_0(...)		SEQ_SIZE_1
#define SEQ_SIZE_1(...)		SEQ_SIZE_2
#define SEQ_SIZE_2(...)		SEQ_SIZE_3
#define SEQ_SIZE_3(...)		SEQ_SIZE_4
#define SEQ_SIZE_4(...)		SEQ_SIZE_5
#define SEQ_SIZE_5(...)		SEQ_SIZE_6
#define SEQ_SIZE_6(...)		SEQ_SIZE_7
#define SEQ_SIZE_7(...)		SEQ_SIZE_8
#define SEQ_SIZE_8(...)		SEQ_SIZE_9
#define SEQ_SIZE_9(...)		SEQ_SIZE_10
#define SEQ_SIZE_10(...)	SEQ_SIZE_11
#define SEQ_SIZE_11(...)	SEQ_SIZE_12
#define SEQ_SIZE_12(...)	SEQ_SIZE_13
#define SEQ_SIZE_13(...)	SEQ_SIZE_14
#define SEQ_SIZE_14(...)	SEQ_SIZE_15
#define SEQ_SIZE_15(...)	SEQ_SIZE_16
#define SEQ_SIZE_16(...)	SEQ_SIZE_17
#define SEQ_SIZE_17(...)	SEQ_SIZE_18
#define SEQ_SIZE_18(...)	SEQ_SIZE_19
#define SEQ_SIZE_19(...)	SEQ_SIZE_20
#define SEQ_SIZE_20(...)	SEQ_SIZE_21
#define SEQ_SIZE_21(...)	SEQ_SIZE_22
#define SEQ_SIZE_22(...)	SEQ_SIZE_23
#define SEQ_SIZE_23(...)	SEQ_SIZE_24
#define SEQ_SIZE_24(...)	SEQ_SIZE_25
#define SEQ_SIZE_25(...)	SEQ_SIZE_26
#define SEQ_SIZE_26(...)	SEQ_SIZE_27
#define SEQ_SIZE_27(...)	SEQ_SIZE_28
#define SEQ_SIZE_28(...)	SEQ_SIZE_29
#define SEQ_SIZE_29(...)	SEQ_SIZE_30
#define SEQ_SIZE_30(...)	SEQ_SIZE_31
#define SEQ_SIZE_31(...)	SEQ_SIZE_32
#define SEQ_SIZE_32(...)	SEQ_SIZE_32
//}}}2
//{{{2 SEQ_SIZE_SEQ_SIZE_N
#define SEQ_SIZE_SEQ_SIZE_0	0
#define SEQ_SIZE_SEQ_SIZE_1	1
#define SEQ_SIZE_SEQ_SIZE_2	2
#define SEQ_SIZE_SEQ_SIZE_3	3
#define SEQ_SIZE_SEQ_SIZE_4	4
#define SEQ_SIZE_SEQ_SIZE_5	5
#define SEQ_SIZE_SEQ_SIZE_6	6
#define SEQ_SIZE_SEQ_SIZE_7	7
#define SEQ_SIZE_SEQ_SIZE_8	8
#define SEQ_SIZE_SEQ_SIZE_9	9
#define SEQ_SIZE_SEQ_SIZE_10	10
#define SEQ_SIZE_SEQ_SIZE_11	11
#define SEQ_SIZE_SEQ_SIZE_12	12
#define SEQ_SIZE_SEQ_SIZE_13	13
#define SEQ_SIZE_SEQ_SIZE_14	14
#define SEQ_SIZE_SEQ_SIZE_15	15
#define SEQ_SIZE_SEQ_SIZE_16	16
#define SEQ_SIZE_SEQ_SIZE_17	17
#define SEQ_SIZE_SEQ_SIZE_18	18
#define SEQ_SIZE_SEQ_SIZE_19	19
#define SEQ_SIZE_SEQ_SIZE_20	20
#define SEQ_SIZE_SEQ_SIZE_21	21
#define SEQ_SIZE_SEQ_SIZE_22	22
#define SEQ_SIZE_SEQ_SIZE_23	23
#define SEQ_SIZE_SEQ_SIZE_24	24
#define SEQ_SIZE_SEQ_SIZE_25	25
#define SEQ_SIZE_SEQ_SIZE_26	26
#define SEQ_SIZE_SEQ_SIZE_27	27
#define SEQ_SIZE_SEQ_SIZE_28	28
#define SEQ_SIZE_SEQ_SIZE_29	29
#define SEQ_SIZE_SEQ_SIZE_30	30
#define SEQ_SIZE_SEQ_SIZE_31	31
#define SEQ_SIZE_SEQ_SIZE_32	32
//}}}2

#define SEQ_SIZE(seq)		PP_CAT_I (SEQ_SIZE_, SEQ_SIZE_0 seq)

#define SEQ_FOR_EACH(seq,d,op)		PP_CAT_I(SEQ_FOR_EACH_, SEQ_SIZE(seq))(seq,d,op)
#define SEQ_FOR_EACH_CALL(x,d,op)	op(d,x)

//{{{2 SEQ_FOR_EACH_N
#define SEQ_FOR_EACH_0(seq,d,op)
#define SEQ_FOR_EACH_1(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op)
#define SEQ_FOR_EACH_2(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_1(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_3(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_2(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_4(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_3(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_5(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_4(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_6(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_5(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_7(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_6(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_8(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_7(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_9(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_8(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_10(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_9(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_11(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_10(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_12(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_11(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_13(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_12(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_14(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_13(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_15(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_14(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_16(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_15(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_17(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_16(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_18(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_17(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_19(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_18(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_20(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_19(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_21(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_20(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_22(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_21(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_23(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_22(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_24(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_23(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_25(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_24(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_26(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_25(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_27(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_26(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_28(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_27(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_29(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_28(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_30(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_29(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_31(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_30(SEQ_POP_FRONT(seq),d,op)
#define SEQ_FOR_EACH_32(seq,d,op)	SEQ_FOR_EACH_CALL(SEQ_HEAD(seq),d,op) SEQ_FOR_EACH_31(SEQ_POP_FRONT(seq),d,op)
//}}}2
//}}}-------------------------------------------------------------------
