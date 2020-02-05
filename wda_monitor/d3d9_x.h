#pragma once

// https://doxygen.reactos.org/de/d57/dll_2directx_2wine_2d3dx9__36_2math_8c_source.html

inline D3DVECTOR* D3DXVec3Add( D3DVECTOR *pOut, CONST D3DVECTOR *pV1, CONST D3DVECTOR *pV2 )
{
	pOut->x = pV1->x + pV2->x;
	pOut->y = pV1->y + pV2->y;
	pOut->z = pV1->z + pV2->z;
	return pOut;
}

inline D3DVECTOR* D3DXVec3Subtract( D3DVECTOR *pOut, CONST D3DVECTOR *pV1, CONST D3DVECTOR *pV2 )
{
	pOut->x = pV1->x - pV2->x;
	pOut->y = pV1->y - pV2->y;
	pOut->z = pV1->z - pV2->z;
	return pOut;
}

inline FLOAT D3DXVec3Dot( CONST D3DVECTOR *pV1, CONST D3DVECTOR *pV2 )
{
	return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

inline D3DVECTOR* D3DXVec3Cross( D3DVECTOR *pOut, CONST D3DVECTOR *pV1, CONST D3DVECTOR *pV2 )
{
	D3DVECTOR v;
	v.x = pV1->y * pV2->z - pV1->z * pV2->y;
	v.y = pV1->z * pV2->x - pV1->x * pV2->z;
	v.z = pV1->x * pV2->y - pV1->y * pV2->x;
	*pOut = v;
	return pOut;
}

inline FLOAT D3DXVec3Length( CONST D3DVECTOR *pV )
{
	return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
}

inline D3DVECTOR* D3DXVec3Normalize( D3DVECTOR *pout, const D3DVECTOR *pv )
{
	FLOAT norm = D3DXVec3Length(pv);
	if ( norm == 0.0f )
	{
		pout->x = 0.0f;
		pout->y = 0.0f;
		pout->z = 0.0f;
	}
	else
	{
		pout->x = pv->x / norm;
		pout->y = pv->y / norm;
		pout->z = pv->z / norm;
	}
	return pout;
}

inline D3DMATRIX* D3DXMatrixIdentity( D3DMATRIX *pOut )
{
	pOut->m[0][1] = pOut->m[0][2] = pOut->m[0][3] =
	pOut->m[1][0] = pOut->m[1][2] = pOut->m[1][3] =
	pOut->m[2][0] = pOut->m[2][1] = pOut->m[2][3] =
	pOut->m[3][0] = pOut->m[3][1] = pOut->m[3][2] = 0.0f;
	pOut->m[0][0] = pOut->m[1][1] = pOut->m[2][2] = pOut->m[3][3] = 1.0f;
	return pOut;
}

inline D3DMATRIX* WINAPI D3DXMatrixRotationY( D3DMATRIX *pout, FLOAT angle )
{
	D3DXMatrixIdentity(pout);
	pout->m[0][0] = cosf(angle);
	pout->m[2][2] = cosf(angle);
	pout->m[0][2] = -sinf(angle);
	pout->m[2][0] = sinf(angle);
	return pout;
}

inline D3DMATRIX* D3DXMatrixLookAtLH( D3DMATRIX *out, const D3DVECTOR *eye, const D3DVECTOR *at, const D3DVECTOR *up )
{
	D3DVECTOR right, upn, vec;
	D3DXVec3Subtract(&vec, at, eye);
	D3DXVec3Normalize(&vec, &vec);
	D3DXVec3Cross(&right, up, &vec);
	D3DXVec3Cross(&upn, &vec, &right);
	D3DXVec3Normalize(&right, &right);
	D3DXVec3Normalize(&upn, &upn);
	out->m[0][0] = right.x;
	out->m[1][0] = right.y;
	out->m[2][0] = right.z;
	out->m[3][0] = -D3DXVec3Dot(&right, eye);
	out->m[0][1] = upn.x;
	out->m[1][1] = upn.y;
	out->m[2][1] = upn.z;
	out->m[3][1] = -D3DXVec3Dot(&upn, eye);
	out->m[0][2] = vec.x;
	out->m[1][2] = vec.y;
	out->m[2][2] = vec.z;
	out->m[3][2] = -D3DXVec3Dot(&vec, eye);
	out->m[0][3] = 0.0f;
	out->m[1][3] = 0.0f;
	out->m[2][3] = 0.0f;
	out->m[3][3] = 1.0f;
	return out;
}

inline D3DMATRIX* D3DXMatrixPerspectiveFovLH( D3DMATRIX *pout, FLOAT fovy, FLOAT aspect, FLOAT zn, FLOAT zf )
{
	D3DXMatrixIdentity(pout);
	pout->m[0][0] = 1.0f / (aspect * tanf(fovy/2.0f));
	pout->m[1][1] = 1.0f / tanf(fovy/2.0f);
	pout->m[2][2] = zf / (zf - zn);
	pout->m[2][3] = 1.0f;
	pout->m[3][2] = (zf * zn) / (zn - zf);
	pout->m[3][3] = 0.0f;
	return pout;
}
