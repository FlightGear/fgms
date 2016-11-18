/*\
 * SGEuler.cxx
 *
 * Licence: GNU GPL version 2
 *
 * Copyright (c) 2015 - Geoff R. McLane
 * code borrowed from http://pigeond.net/git/?p=flightgear/fgmap.git;a=blob_plain;f=sg_perl/sgmath.cxx;hb=HEAD
 * Copyright (c) 2015 - Pigeond
\*/

#include <limits>
#include <math.h>
#ifndef _MSC_VER
#include <complex.h>
#endif

// Codes borrowed from simgear with unneeded stuff removed

static const double SGD_PI = M_PI;
static const double SGD_DEGREES_TO_RADIANS = SGD_PI / 180.0;
static const float SGMISC_PI = float(3.1415926535897932384626433832795029L);

struct SGVec3f;
static float norm(const SGVec3f &v);

static float rad2deg(const float &val)
{
	return val * 180 / SGMISC_PI;
}

struct SGVec3f {
	SGVec3f() {}
	SGVec3f(float x, float y, float z) { data()[0] = x; data()[1] = y; data()[2] = z; }

	const float (&data(void) const)[3] { return _data; }
	float (&data(void))[3] { return _data; }

	const float &operator()(unsigned i) const { return data()[i]; }
	float &operator()(unsigned i) { return data()[i]; }

	float &x(void) { return data()[0]; }
	float &y(void) { return data()[1]; }
	float &z(void) { return data()[2]; }
	const float &x(void) const { return data()[0]; }
	const float &y(void) const { return data()[1]; }
	const float &z(void) const { return data()[2]; }

	float _data[3];
};

SGVec3f operator*(float s, const SGVec3f &v) {
	return SGVec3f(s * v(0), s * v(1), s * v(2));
}


struct SGQuatf {
	SGQuatf() {}
	SGQuatf(float _x, float _y, float _z, float _w)
	{ x() = _x; y() = _y; z() = _z; w() = _w; }

	const float (&data(void) const)[4] { return _data; }
	float (&data(void))[4] { return _data; }

	const float &operator()(unsigned i) const { return data()[i]; }
	float &operator()(unsigned i) { return data()[i]; }

	float &x(void) { return data()[0]; }
	float &y(void) { return data()[1]; }
	float &z(void) { return data()[2]; }
	float &w(void) { return data()[3]; }
	const float &x(void) const { return data()[0]; }
	const float &y(void) const { return data()[1]; }
	const float &z(void) const { return data()[2]; }
	const float &w(void) const { return data()[3]; }

	float _data[4];

	static SGQuatf fromAngleAxis(const SGVec3f axis)
	{
		float nAxis = norm(axis);
		if (nAxis <= std::numeric_limits<float>::min()) {
			return SGQuatf::unit();
		}
		float angle2 = float(0.5) * nAxis;
		return fromRealImag(cos(angle2), float(sin(angle2) / nAxis) * axis);
	}

	static SGQuatf unit(void)
	{
		return fromRealImag(1, SGVec3f(0, 0, 0));
	}

	static SGQuatf fromRealImag(float r, const SGVec3f &i)
	{
		SGQuatf q;
		q.w() = r;
		q.x() = i.x();
		q.y() = i.y();
		q.z() = i.z();
		return q;
	}

	static SGQuatf fromLonLatRad(float lon, float lat)
	{
		SGQuatf q;
		float zd2 = float(0.5) * lon;
		float yd2 = float(-0.25) * float(SGMISC_PI) - float(0.5) * lat;
		float Szd2 = sin(zd2);
		float Syd2 = sin(yd2);
		float Czd2 = cos(zd2);
		float Cyd2 = cos(yd2);
		q.w() = Czd2 * Cyd2;
		q.x() = -Szd2 * Syd2;
		q.y() = Czd2 * Syd2;
		q.z() = Szd2 * Cyd2;
		return q;
	}

	void getEulerRad(float &zRad, float &yRad, float &xRad) const
	{
		float sqrQW = w() * w();
		float sqrQX = x() * x();
		float sqrQY = y() * y();
		float sqrQZ = z() * z();
		
		float num = 2 * (y() * z() + w() * x());
		float den = sqrQW - sqrQX - sqrQY + sqrQZ;
		if (fabs(den) <= std::numeric_limits<float>::min() &&
				fabs(num) <= std::numeric_limits<float>::min())
			xRad = 0;
		else
			xRad = atan2(num, den);
		
		float tmp = 2 * (x() * z() - w() * y());
		if (tmp <= -1)
			yRad = float(0.5) * SGMISC_PI;
		else if (1 <= tmp)
			yRad =  float(0.5) * SGMISC_PI;
		else
			yRad = -asin(tmp);
		
		num = 2 * (x() * y() + w() * z());
		den = sqrQW + sqrQX - sqrQY - sqrQZ;
		if (fabs(den) <= std::numeric_limits<float>::min() &&
				fabs(num) <= std::numeric_limits<float>::min()) {
			zRad = 0;
		} else {
			float psi = atan2(num, den);
			if (psi < 0)
				psi += 2 * SGMISC_PI;
			zRad = psi;
		}
	}


	void getEulerDeg(float &zDeg, float &yDeg, float &xDeg) const
	{
		getEulerRad(zDeg, yDeg, xDeg);
		zDeg = rad2deg(zDeg);
		yDeg = rad2deg(yDeg);
		xDeg = rad2deg(xDeg);
	}
};

SGQuatf
operator*(const SGQuatf &v1, const SGQuatf &v2)
{
	SGQuatf v;
	v.x() = v1.w() * v2.x() + v1.x() * v2.w() + v1.y() * v2.z() - v1.z() * v2.y();
	v.y() = v1.w() * v2.y() - v1.x() * v2.z() + v1.y() * v2.w() + v1.z() * v2.x();
	v.z() = v1.w() * v2.z() + v1.x() * v2.y() - v1.y() * v2.x() + v1.z() * v2.w();
	v.w() = v1.w() * v2.w() - v1.x() * v2.x() - v1.y() * v2.y() - v1.z() * v2.z();
	return v;
}


static float dot(const SGVec3f &v1, const SGVec3f &v2)
{
	return v1(0) * v2(0) + v1(1) * v2(1) + v1(2) * v2(2);
}

static float norm(const SGVec3f &v)
{
	return sqrt(dot(v, v));
}

static SGQuatf conj(const SGQuatf &v)
{
	return SGQuatf(-v(0), -v(1), -v(2), v(3));
}

void euler_get(float lat, float lon, float ox, float oy, float oz,
                float *head, float *pitch, float *roll)
{
    /* FGMultiplayMgr::ProcessPosMsg */

    SGVec3f angleAxis;
    angleAxis(0) = ox;
    angleAxis(1) = oy;
    angleAxis(2) = oz;

    SGQuatf ecOrient;
    ecOrient = SGQuatf::fromAngleAxis(angleAxis);

    /* FGAIMultiplayer::update */

    float lat_rad, lon_rad;
    lat_rad = (float)(lat * SGD_DEGREES_TO_RADIANS);
    lon_rad = (float)(lon * SGD_DEGREES_TO_RADIANS);

    SGQuatf qEc2Hl = SGQuatf::fromLonLatRad(lon_rad, lat_rad);

    SGQuatf hlOr = conj(qEc2Hl) * ecOrient;

    float hDeg, pDeg, rDeg;
    hlOr.getEulerDeg(hDeg, pDeg, rDeg);

    if(head)
        *head = hDeg;
    if(pitch)
        *pitch = pDeg;
    if(roll)
        *roll = rDeg;
}

/* eof */
