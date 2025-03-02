/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "data_storage/Value.h"
#include "data_storage/BasicAnnotations.h"
#include "data_storage/BasicTypes.h"
#include "data_storage/Table.h"

#include "StructTypes.h"

#include "gtest/gtest.h"

using namespace raco::data_storage;


TEST(Vec3f, has3Children) {
	Vec3f underTest{0.0, 0.0, 0.0, 0.0};
	EXPECT_EQ(underTest.size(), 3);
}

TEST(ValueTest, Scalar) {

	Value<bool> bv;
	Value<int> iv;

	Value<double> dv;
	dv = 2.0;
	EXPECT_EQ(*dv, 2.0);

	EXPECT_EQ(dv.asDouble(), 2.0);
	EXPECT_EQ(dv.as<double>(), 2.0);

	EXPECT_THROW(dv.asBool(), std::runtime_error);
	EXPECT_THROW(dv.as<bool>(), std::runtime_error);


	Value<double> u;
	u = 3.0;
	u = dv;
	

	Value<std::string> s;

}

TEST(ValueTest, Tables)
{
	Value<Table> tv;
	EXPECT_EQ(tv->size(), 0);

	tv->addProperty("intval", PrimitiveType::Int);
	EXPECT_EQ(tv->size(), 1);
	EXPECT_EQ(tv->index("intval"), 0);

	EXPECT_EQ(tv->name(0), "intval");
	EXPECT_EQ((*tv)[0]->asInt(), 0);
	EXPECT_EQ((*tv)["intval"]->asInt(), 0);
	EXPECT_EQ(tv->get("intval")->asInt(), 0);

	tv->addProperty("fval", PrimitiveType::Double);
	EXPECT_EQ(tv->size(), 2);
	EXPECT_EQ(tv->index("fval"), 1);

	EXPECT_EQ(tv->name(1), "fval");
	EXPECT_EQ((*tv)[1]->asDouble(), 0.0);
	EXPECT_EQ((*tv)["fval"]->asDouble(), 0.0);
	EXPECT_EQ(tv->get("fval")->asDouble(), 0.0);
	EXPECT_THROW((*tv)[1]->asInt(), std::runtime_error);

	(*tv)[0]->asInt() = 42;
	EXPECT_EQ(tv->get("intval")->asInt(), 42);

	*tv->get("fval") = 2.0;
	EXPECT_EQ((*tv)[1]->asDouble(), 2.0);

	// Check Table assignment operator
	Value<Table> tu;
	tu = tv;

	EXPECT_EQ(tu->size(), tv->size());
	EXPECT_EQ(tu->get("intval")->asInt(), 42);
	EXPECT_EQ(tv->get("fval")->asDouble(), 2.0);
}

TEST(ValueTest, Vec3f) {
	Value<Vec3f> v;

	v->x = 2.0;
	EXPECT_EQ(*(v->x), 2.0);

	double a = *v->x;
	EXPECT_EQ(a, 2.0);

	Vec3f u { 3.0 };
	v = u;
	EXPECT_EQ(*(v->x), 3.0);
	EXPECT_EQ(*(v->y), 3.0);
	EXPECT_EQ(*(v->z), 3.0);


	ValueBase* vp = &v;

	EXPECT_EQ(*vp->as<Vec3f>().x, 3.0);
	EXPECT_THROW(vp->as<bool>(), std::runtime_error);

}

TEST(ValueTest, clone) {
	Value<int> vint {23};
	auto vint_clone = vint.clone(nullptr);
	EXPECT_EQ(vint_clone->asInt(), *vint);

	Value<Table> vt;
	vt->addProperty("test", PrimitiveType::Double);

	auto vt_clone = vt.clone(nullptr);

	EXPECT_EQ(vt_clone->asTable().size(), vt->size());
	EXPECT_EQ(vt_clone->asTable().name(0), vt->name(0));
}

TEST(ValueTest, Struct) {
	SimpleStruct s;
	s.bb = false;
	s.dd = 2.0;

	Value<SimpleStruct> vs;
	const Value<SimpleStruct> cvs(s);
	Value<AltStruct> va;

	EXPECT_EQ(vs.type(), PrimitiveType::Struct);
	EXPECT_EQ(vs.typeName(), "SimpleStruct");

	EXPECT_THROW(vs.asBool(), std::runtime_error);
	EXPECT_THROW(vs.as<bool>(), std::runtime_error);
	EXPECT_THROW(vs.asVec3f(), std::runtime_error);
	EXPECT_THROW(vs.as<Vec3f>(), std::runtime_error);

	ReflectionInterface& intf = vs.getSubstructure();
	EXPECT_EQ(intf.size(), 2);

	const ReflectionInterface& cintf = cvs.getSubstructure();
	EXPECT_EQ(cintf.size(), 2);

	EXPECT_EQ(vs->size(), 2);

	EXPECT_THROW(vs.asRef(), std::runtime_error);

	// operator=
	vs = cvs;
	EXPECT_EQ(*vs->dd, 2.0);

	Value<Vec3f> v;
	EXPECT_THROW(vs = v, std::runtime_error);

	EXPECT_THROW(vs = va, std::runtime_error);

	vs = s;
	EXPECT_TRUE(*vs == s);

	// operator==
	EXPECT_TRUE(vs == cvs);
	EXPECT_FALSE(vs == va);

	// assign
	EXPECT_FALSE(vs.assign(cvs));
	EXPECT_THROW(vs.assign(va), std::runtime_error);

	// copyAnnotationData
	EXPECT_NO_THROW(vs.copyAnnotationData(cvs));
	EXPECT_THROW(vs.copyAnnotationData(va), std::runtime_error);

	// clone
	auto vsclone = vs.clone(nullptr);

	EXPECT_TRUE(vs == *vsclone.get());
}
