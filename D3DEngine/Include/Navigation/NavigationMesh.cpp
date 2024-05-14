#include "NavigationMesh.h"

ENGINE_USING

CNavigationMesh::CNavigationMesh()
{
	m_bGrid = false;
	m_iLineRectCount = 0;
	m_iSectionX = 1;
	m_iSectionZ = 1;
	m_pSection = new NavSection[m_iSectionX * m_iSectionZ];

	m_iUseCellListSize = 0;
	m_pUseCellList = nullptr;

	m_OpenList.SetSortFunc(this, &CNavigationMesh::OpenListSort);
}

CNavigationMesh::~CNavigationMesh()
{
	SAFE_DELETE_ARRAY(m_pUseCellList);
	SAFE_DELETE_ARRAY(m_pSection);
	Safe_Delete_VecList(m_vecCell);
}

Vector3 CNavigationMesh::GetMin() const
{
	return m_vMin;
}

Vector3 CNavigationMesh::GetMax() const
{
	return m_vMax;
}

Vector3 CNavigationMesh::GetOffset() const
{
	return m_vOffset;
}

list<Vector3> CNavigationMesh::GetPathList() const
{
	return m_PathList;
}

void CNavigationMesh::SetOffset(const Vector3 & vOffset)
{
	m_vOffset = vOffset;
}

void CNavigationMesh::SetOffsetScale(const Vector3 & vOffsetScale)
{
	m_vOffsetScale = vOffsetScale;
}

bool CNavigationMesh::Init()
{
	m_vMin.x = 10000000.f;
	m_vMin.y = 10000000.f;
	m_vMin.z = 10000000.f;

	m_vMax.x = -10000000.f;
	m_vMax.y = -10000000.f;
	m_vMax.z = -10000000.f;

	return true;
}

void CNavigationMesh::AddCell(const Vector3 vPos[3])
{
	PNavigationCell	pCell = new NavigationCell;

	for (int i = 0; i < 3; ++i)
	{
		pCell->vPos[i] = vPos[i];
	}

	pCell->vEdge[0] = vPos[1] - vPos[0];
	pCell->vEdgeCenter[0] = (vPos[1] + vPos[0]) / 2.f;

	pCell->vEdge[1] = vPos[2] - vPos[1];
	pCell->vEdgeCenter[1] = (vPos[2] + vPos[1]) / 2.f;

	pCell->vEdge[2] = vPos[2] - vPos[0];
	pCell->vEdgeCenter[2] = (vPos[2] + vPos[0]) / 2.f;

	pCell->vCenter = (vPos[0] + vPos[1] + vPos[2]) / 3.f;

	pCell->iIndex = (int)m_vecCell.size();

	for (int i = 0; i < 3; ++i)
	{
		if (m_vMin.x > vPos[i].x)
			m_vMin.x = vPos[i].x;
		if (m_vMin.y > vPos[i].y)
			m_vMin.y = vPos[i].y;
		if (m_vMin.z > vPos[i].z)
			m_vMin.z = vPos[i].z;

		if (m_vMax.x < vPos[i].x)
			m_vMax.x = vPos[i].x;
		if (m_vMax.y < vPos[i].y)
			m_vMax.y = vPos[i].y;
		if (m_vMax.z < vPos[i].z)
			m_vMax.z = vPos[i].z;
	}

	Vector3	vDir1 = pCell->vEdge[0];
	Vector3	vDir2 = pCell->vEdge[2];

	vDir1.Normalize();
	vDir2.Normalize();

	// �������͸� ���Ѵ�.
	Vector3	vFaceNormal = vDir1.Cross(vDir2);
	vFaceNormal.Normalize();

	Vector3	vView = Vector3::Axis[AXIS_Y];

	// World Y�� �������� ������ Angle���� ���Ѵ�.
	float	fAngle = vView.Angle(vFaceNormal);

	// Angle���� 65.f ���� ũ�� ������ �����Ƿ� Cell�� ����.
	if (fAngle >= 65.f)
		pCell->bEnable = false;

	m_vecCell.push_back(pCell);
}

void CNavigationMesh::AddAdj(int iCellIdx, int iAdjIdx)
{
	if (iCellIdx < 0 || iCellIdx >= m_vecCell.size())
		return;

	ADJINFO	tInfo = {};
	tInfo.iIndex = iAdjIdx;
	tInfo.iEdgeIndex = 0;
	m_vecCell[iCellIdx]->vecAdj.push_back(tInfo);
}

void CNavigationMesh::CreateGridMapAdj(int iLineRectCount)
{
	m_bGrid = true;
	m_iLineRectCount = iLineRectCount;

	size_t iSize = m_vecCell.size();
	ADJINFO	tInfo = {};

	for (int i = 0; i < iSize; i += 2)
	{
		// ���� �ﰢ�� �������� ����.
		// ���� �簢��, ������ �簢��, �ڱ��ڽ� �簢���� ���� �ϴ� �ﰢ����
		// �������� �ĺ��� �ȴ�.
		// ���� �簢���� ���ϴ� �ﰢ�� �ε����� ���Ѵ�.
		int	idx = i - iLineRectCount * 2 + 1;
		if (idx >= 0 && idx < m_vecCell.size())
		{
			tInfo.iIndex = idx;
			tInfo.iEdgeIndex = 0;
			m_vecCell[i]->vecAdj.push_back(tInfo);
		}

		// �ڱ��ڽ� �簢���� ���� �ϴ� �ﰢ�� �ε����� ���Ѵ�.
		idx = i + 1;
		if (idx >= 0 && idx < m_vecCell.size())
		{
			tInfo.iIndex = idx;
			tInfo.iEdgeIndex = 2;
			m_vecCell[i]->vecAdj.push_back(tInfo);
		}

		// ������ �簢���� ���� �ϴ� �ﰢ�� �ε����� ���Ѵ�.
		if ((i / 2) % iLineRectCount != iLineRectCount - 1)
		{
			idx = i + 3;
			tInfo.iIndex = idx;
			tInfo.iEdgeIndex = 1;
			m_vecCell[i]->vecAdj.push_back(tInfo);
		}

		// ���ϴ� �ﰢ�� �������� ����.
		// �Ʒ��� �簢��, ���� �簢��, �ڱ��ڽ� �簢���� ������ ��� �ﰢ����
		// �������� �ĺ��� �ȴ�.
		// �ڱ��ڽ� �簢���� ���� �ﰢ�� �ε����� ���Ѵ�.
		tInfo.iIndex = (int)i;
		tInfo.iEdgeIndex = 0;
		m_vecCell[i + 1]->vecAdj.push_back(tInfo);

		// ���� �簢���� ���� �ﰢ�� �ε����� ���Ѵ�.
		if ((i / 2) % iLineRectCount != 0)
		{
			tInfo.iIndex = i - 3;
			tInfo.iEdgeIndex = 2;
			m_vecCell[i + 1]->vecAdj.push_back(tInfo);
		}

		idx = (int)i + (iLineRectCount * 2);
		if (idx < m_vecCell.size())
		{
			tInfo.iIndex = idx;
			tInfo.iEdgeIndex = 1;
			m_vecCell[i + 1]->vecAdj.push_back(tInfo);
		}
	}

	CreateSection();
}

void CNavigationMesh::CreateAdj()
{
	size_t	iSize = m_vecCell.size();

	Vector3	vOrigin[2][3];
	Vector3	vEdgeDir[2][3];
	float	fLength[2][3];

	for (int i = 0; i < iSize; ++i)
	{
		Vector3	vSrc[3];
		Vector3	vSrcOrigin[3];
		float	fSrcLength[3] = {};

		for (int k = 0; k < 3; ++k)
		{
			vEdgeDir[0][k] = m_vecCell[i]->vEdge[k];
			fLength[0][k] = vEdgeDir[0][k].Length();
			vEdgeDir[0][k].Normalize();
		}

		vOrigin[0][0] = m_vecCell[i]->vPos[0];
		vOrigin[0][1] = m_vecCell[i]->vPos[1];
		vOrigin[0][2] = m_vecCell[i]->vPos[2];

		for (int j = i + 1; j < iSize; ++j)
		{
			// �ﰢ���� 3���� �������� ���� �����Ͽ� �������� ���Ѵ�.
			Vector3	vDest[3];
			//float fDestLength[3];
			Vector3	vDestOrigin[3];
			int	iPosIdx[3][2] = { 1, 0, 2, 1, 2, 0 };
			bool	bInsert = false;

			vOrigin[1][0] = m_vecCell[j]->vPos[0];
			vOrigin[1][1] = m_vecCell[j]->vPos[1];
			vOrigin[1][2] = m_vecCell[j]->vPos[2];

			for (int k = 0; k < 3; ++k)
			{
				vEdgeDir[1][k] = m_vecCell[j]->vEdge[k];

				fLength[1][k] = vEdgeDir[1][k].Length();

				vEdgeDir[1][k].Normalize();
			}

			for (int k = 0; k < 3; ++k)
			{
				if (bInsert)
					break;

				for (int l = 0; l < 3; ++l)
				{
					// A�ﰢ���� B�ﰢ���� ������ �����ϴ� �� ������ ���� ���
					// �� �ﰢ���� ������ �پ��ִ� �ﰢ���̴�.
					if ((vOrigin[0][iPosIdx[k][0]] == vOrigin[1][iPosIdx[l][0]] ||
						vOrigin[0][iPosIdx[k][0]] == vOrigin[1][iPosIdx[l][1]]) &&
						(vOrigin[0][iPosIdx[k][1]] == vOrigin[1][iPosIdx[l][0]] ||
							vOrigin[0][iPosIdx[k][1]] == vOrigin[1][iPosIdx[l][1]]))
					{
						ADJINFO	tInfo = {};
						tInfo.iIndex = (int)j;
						tInfo.iEdgeIndex = k;
						m_vecCell[i]->vecAdj.push_back(tInfo);

						tInfo.iIndex = (int)i;
						tInfo.iEdgeIndex = l;
						m_vecCell[j]->vecAdj.push_back(tInfo);
						bInsert = true;
						break;
					}

					// �� ������ ���ؼ� ��� ������ �����ϴ� 2���� ���� ��� ����
					// ���� �Ѵ� �����ϴ����� �Ǵ��Ѵ�. �Ѵ� �����Ѵٸ�
					// ������ �پ��ִ� ���̴�.
					// (B1 - A1, B2 - A1), (B1 - A2, B2 - A2)
					// ������ ���ؼ� ���� ������ ũ�Ⱑ �״�� ���´ٸ�
					// ������ �����ϴ� ���̴�. ��, �Ѵ� ũ�Ⱑ ���� ����
					// ������ �����ϴ� ���̰� ���� �ϳ��� ���´ٸ� ũ�Ⱑ 0�̶��
					// �ϳ��� ���� �������� �����ϰ� �� �� ��ġ�� ���ٴ� ���̹Ƿ�
					// �ش� �ﰢ���� ���� �ﰢ���� �ƴϴ�.
					// B1 - A1 ó��
					if (CheckOnEdge(i, j, vOrigin[1][iPosIdx[l][0]], vOrigin[0][iPosIdx[k][0]], vEdgeDir[0][k], fLength[0][k], k, l))
					{
						bInsert = true;
						break;
					}

					// B2 - A1 ó��
					else if (CheckOnEdge(i, j, vOrigin[1][iPosIdx[l][1]], vOrigin[0][iPosIdx[k][0]], vEdgeDir[0][k], fLength[0][k], k, l))
					{
						bInsert = true;
						break;
					}

					// B1 - A2 ó��
					else if (CheckOnEdge(i, j, vOrigin[1][iPosIdx[l][0]], vOrigin[0][iPosIdx[k][1]], vEdgeDir[0][k] * -1.f, fLength[0][k], k, l))
					{
						bInsert = true;
						break;
					}

					// B2 - A2 ó��
					else if (CheckOnEdge(i, j, vOrigin[1][iPosIdx[l][1]], vOrigin[0][iPosIdx[k][1]], vEdgeDir[0][k] * -1.f, fLength[0][k], k, l))
					{
						bInsert = true;
						break;
					}
				}
			}
		}
	}
	CreateSection();
}

bool CNavigationMesh::CheckOnEdge(int iSrc, int iDest, const Vector3 & vOrigin1, const Vector3 & vOrigin2, const Vector3 & vEdge, float fEdgeLength, int iEdge1, int iEdge2)
{
	if ((int)(vOrigin2.x * 100) == (int)(vOrigin1.x * 100) &&
		(int)(vOrigin2.y * 100) == (int)(vOrigin1.y * 100) &&
		(int)(vOrigin2.z * 100) == (int)(vOrigin1.z * 100))
		return false;

	Vector3	vResult = vOrigin1 - vOrigin2;

	float	fDist = vResult.Length();

	// ������ ������ �Ÿ��� �̿��ؼ� �������κ��� ���� �������� ������ �Ÿ���ŭ �̵��� ��ġ�� ��� ���� �ִ�����
	// �Ǵ��Ѵ�.
	vResult = vOrigin2 + vEdge * fDist;

	if ((int)(vResult.x * 100) == (int)(vOrigin1.x * 100) &&
		(int)(vResult.y * 100) == (int)(vOrigin1.y * 100) &&
		(int)(vResult.z * 100) == (int)(vOrigin1.z * 100))
	{
		vResult -= vOrigin2;
		if (vResult.Length() < fEdgeLength)
		{
			ADJINFO	tInfo = {};
			tInfo.iIndex = iDest;
			tInfo.iEdgeIndex = iEdge1;
			m_vecCell[iSrc]->vecAdj.push_back(tInfo);

			tInfo.iIndex = iSrc;
			tInfo.iEdgeIndex = iEdge2;
			m_vecCell[iDest]->vecAdj.push_back(tInfo);
			return true;
		}
	}

	return false;
}

bool CNavigationMesh::CheckPathDir(const Vector3 & vSrc1, const Vector3 & vSrc2, const Vector3 & vDest1, const Vector3 & vDest2, Vector3 & vIntersect)
{
	return false;
}

void CNavigationMesh::FindPath(const Vector3 & vStart, const Vector3 & vEnd)
{
	PNavigationCell pStart = FindCell(vStart);
	PNavigationCell pEnd = FindCell(vEnd);

	if (!pStart || !pEnd)
		return;
	else if (!pEnd->bEnable)
		return;

	if (pStart == pEnd)
	{
		m_PathList.clear();
		m_PathList.push_back(vEnd);
		return;
	}

	if (!m_pUseCellList)
		m_pUseCellList = new PNavigationCell[m_vecCell.size()];
	for (int i = 0; i < m_iUseCellListSize; ++i)
	{
		m_pUseCellList[i]->eType = NCLT_NONE;
		m_pUseCellList[i]->iParentIdx = -1;
		m_pUseCellList[i]->fG = -1.f;
		m_pUseCellList[i]->fH = -1.f;
		m_pUseCellList[i]->fTotal = -1.f;
	}

	m_iUseCellListSize = 0;

	while (!m_FindStack.empty())
	{
		m_FindStack.pop();
	}

	m_OpenList.Clear();

	// ���۳�带 ������Ͽ� �־��ش�.
	pStart->eType = NCLT_OPEN;
	pStart->fG = 0.f;
	pStart->fH = vStart.Distance(vEnd);
	pStart->fTotal = pStart->fH;

	m_OpenList.Insert(pStart);

	m_bFindEnd = false;

	PNavigationCell	pCell = nullptr;

	while (!m_OpenList.empty() && !m_bFindEnd)
	{
		// ������Ͽ��� ���� ���´�.
		m_OpenList.Pop(pCell);

		// ���� ���� ����������� ������ش�.
		pCell->eType = NCLT_CLOSE;

		AddOpenList(pCell, pEnd, vStart, vEnd);
	}

}

float CNavigationMesh::GetY(const Vector3 & vPos)
{
	if (!m_bGrid)
	{
		int	iSectionIndex = GetSectionIndex(vPos);

		if (0 <= iSectionIndex && iSectionIndex < m_iSectionX * m_iSectionZ)
		{
			Vector3	vOrigin = vPos;
			vOrigin.y = m_vMax.y;
			Vector3	vDir = Vector3(0.f, -1.f, 0.f);

			for (int i = 0; i < m_pSection[iSectionIndex].tCellList.iSize; ++i)
			{
				Vector3	vIntersect;
				float	fDist = 0.f;
				PNavigationCell	pCell = m_pSection[iSectionIndex].tCellList.pCellList[i];

				if (RayIntersectTriangle(vOrigin, vDir, pCell->vPos[0],	pCell->vPos[1],	pCell->vPos[2],	fDist, vIntersect))
					return vIntersect.y;
			}
		}

		return 0.f;
	}

	Vector3	vCellSize = (m_vMax - m_vMin) / m_iLineRectCount;
	Vector3	vConvertPos = vPos - m_vMin;

	// ����, ���θ� 1�� ������ش�.
	vConvertPos.x /= vCellSize.x;
	vConvertPos.z /= vCellSize.z;

	// �簢�� �ε����� ���Ѵ�.
	int	idxX = (int)vConvertPos.x;
	int	idxZ = m_iLineRectCount - ((int)vConvertPos.z + 1);

	if (idxX < 0 || idxX >= m_iLineRectCount)
		return 0.f;

	else if (idxZ < 0 || idxZ >= m_iLineRectCount)
		return 0.f;

	int	idx = (idxZ * m_iLineRectCount + idxX) * 2;

	if (!m_vecCell[idx]->bEnable)
		return 0;

	// ������ �簢���� �»�� ���� ���Ѵ�.
	Vector3	vLTPos = m_vecCell[idx]->vPos[0];
	vLTPos.x /= vCellSize.x;
	vLTPos.z /= vCellSize.z;

	float	fX = vConvertPos.x - vLTPos.x;
	float	fZ = vLTPos.z - vConvertPos.z;

	// ���� �ﰢ���� ���
	if (fX >= fZ)
	{
		// Cell�� �����ϴ� ���� Y���� ���´�.
		float	fY[3] = {};
		for (int i = 0; i < 3; ++i)
		{
			fY[i] = m_vecCell[idx]->vPos[i].y;
		}

		return fY[0] + (fY[1] - fY[0]) * fX + (fY[2] - fY[1]) * fZ;
	}

	// ���ϴ� �ﰢ���� ���
	float	fY[3] = {};
	for (int i = 0; i < 3; ++i)
	{
		fY[i] = m_vecCell[idx + 1]->vPos[i].y;
	}

	return fY[0] + (fY[1] - fY[2]) * fX + (fY[2] - fY[0]) * fZ;
}

bool CNavigationMesh::CheckCell(const Vector3 & vPos)
{
	int	iCellIndex = GetCellIndex(vPos);

	if (iCellIndex == -1)
		return false;

	float	fY = GetY(iCellIndex, vPos);

	if (!m_vecCell[iCellIndex]->bEnable || (vPos.y - 2.f > fY || fY > vPos.y + 2.f)) 
		return false;

	return true;
}

bool CNavigationMesh::GetCellEnable(const Vector3 & vPos)
{
	int	iCellIndex = GetCellIndex(vPos);

	if (iCellIndex == -1)
		return false;

	return m_vecCell[iCellIndex]->bEnable;
}

float CNavigationMesh::GetY(int iCellIndex, const Vector3 & vPos)
{
	return 0.0f;
}

void CNavigationMesh::CreateSection()
{
	m_iSectionX = 5;
	m_iSectionZ = 5;

	Vector3	vLength = m_vMax - m_vMin;
	m_vSectionSize = vLength / Vector3(m_iSectionX, 1, m_iSectionZ);

	SAFE_DELETE_ARRAY(m_pSection);

	m_pSection = new NavSection[m_iSectionX * m_iSectionZ];

	for (int i = 0; i < m_iSectionZ; ++i)
	{
		for (int j = 0; j < m_iSectionX; ++j)
		{
			int	idx = i * m_iSectionX + j;
			m_pSection[idx].vSize = m_vSectionSize;
			m_pSection[idx].vMin = m_vMin + m_vSectionSize * Vector3(j, 0, i);
			m_pSection[idx].vMax = m_vMin + m_vSectionSize * Vector3((j + 1), 1, (i + 1));
		}
	}

	// �ִ� 4���� ������ ���Ե� �� �ִ�.
	// ��� ���ԵǾ� �ִ��� �Ǵ��ؾ� �Ѵ�. ����.
	for (size_t i = 0; i < m_vecCell.size(); ++i)
	{
		// �ﰢ���� �����ϴ� 3���� ���� �̿��ؼ� �ε����� ���Ѵ�.
		int	iMinX = 100000, iMinZ = 100000;
		int	iMaxX = -100000, iMaxZ = -100000;

		for (int j = 0; j < 3; ++j)
		{
			Vector3	vPos = m_vecCell[i]->vPos[j] - m_vMin;
			vPos /= m_vSectionSize;

			int x, z;
			x = (int)vPos.x;
			z = (int)vPos.z;

			if (iMinX > x)
				iMinX = x;

			if (iMaxX < x)
				iMaxX = x;

			if (iMinZ > z)
				iMinZ = z;

			if (iMaxZ < z)
				iMaxZ = z;
		}

		iMinX = iMinX < 0 ? 0 : iMinX;
		iMinZ = iMinZ < 0 ? 0 : iMinZ;
		iMaxX = iMaxX >= m_iSectionX ? m_iSectionX - 1 : iMaxX;
		iMaxZ = iMaxZ >= m_iSectionZ ? m_iSectionZ - 1 : iMaxZ;

		if (iMinX < m_iSectionX && iMaxX >= 0 &&
			iMinZ < m_iSectionZ && iMaxZ >= 0)
		{
			for (int j = iMinZ; j <= iMaxZ; ++j)
			{
				for (int k = iMinX; k <= iMaxX; ++k)
				{
					int	idx = j * m_iSectionX + k;

					m_pSection[idx].Add(m_vecCell[i]);
				}
			}
		}
	}
}

void CNavigationMesh::AddOpenList(PNavigationCell pCell, PNavigationCell pEnd, const Vector3 & vStart, const Vector3 & vEnd)
{
	for (size_t i = 0; i < pCell->vecAdj.size(); ++i)
	{
		if (pCell->vecAdj[i].iIndex == -1)
			continue;

		PNavigationCell	pAdj = m_vecCell[pCell->vecAdj[i].iIndex];

		if (!pAdj->bEnable)
			continue;

		else if (pAdj->eType == NCLT_CLOSE)
			continue;

		// ���� ���� ã���� ���
		if (pAdj == pEnd)
		{
			// �θ��� �ε����� ���´�.
			int	iParent = pCell->iIndex;

			while (iParent != -1)
			{
				m_FindStack.push(iParent);
				iParent = m_vecCell[iParent]->iParentIdx;
			}

			// ���۳��� �������ش�.
			m_FindStack.pop();

			// �������� ������� ��θ� ������� �ǵ�����.
			vector<int>	vecPathIndex;

			while (!m_FindStack.empty())
			{
				int	idx = m_FindStack.top();
				m_FindStack.pop();
				vecPathIndex.push_back(idx);
			}

			m_PathList.clear();

			vector<int>	vecCenter;
			for (size_t j = 0; j < vecPathIndex.size()-1; ++j)
			{
				int	iEdgeIndex = -1;

				pCell = m_vecCell[vecPathIndex[j]];

				for (size_t k = 0; k < pCell->vecAdj.size(); ++k)
				{
					if (pCell->vecAdj[k].iIndex == vecPathIndex[j + 1])
					{
						iEdgeIndex = pCell->vecAdj[k].iEdgeIndex;
						break;
					}
				}

				// ������ ��� ������ �������� ���������� �����Ǿ�����
				// ã�Ҵٸ� ���⼭ ���������� �����ش�.
				vecCenter.push_back(iEdgeIndex);

				m_PathList.push_back(pCell->vEdgeCenter[iEdgeIndex]);
			}

			// ������ġ�� �־��ش�.
			m_PathList.push_back(vEnd);

			m_bFindEnd = true;

			return;
		}

		float	fG = pAdj->vCenter.Distance(pCell->vCenter);
		float	fH = pAdj->vCenter.Distance(vEnd);

		if (pAdj->eType == NCLT_NONE)
		{
			m_pUseCellList[m_iUseCellListSize] = pAdj;
			++m_iUseCellListSize;

			pAdj->fG = fG + pCell->fG;
			pAdj->fH = fH;
			pAdj->fTotal = pAdj->fG + pAdj->fH;
			pAdj->iParentIdx = pCell->iIndex;
			pAdj->eType = NCLT_OPEN;

			m_OpenList.Insert(pAdj);
		}

		// ������Ͽ� ���ִ� Cell�� ���
		else if (pAdj->fG > fG + pCell->fG)
		{
			pAdj->fG = fG + pCell->fG;
			pAdj->fH = fH;
			pAdj->fTotal = pAdj->fG + pAdj->fH;
			pAdj->iParentIdx = pCell->iIndex;
			m_OpenList.Sort();
		}
	}
}

PNavigationCell CNavigationMesh::FindCell(const Vector3 & vPos)
{
	int	idx = GetCellIndex(vPos);

	if (idx < 0 || idx >= m_vecCell.size())
		return nullptr;

	return m_vecCell[idx];
}

int CNavigationMesh::GetCellIndex(const Vector3 & vPos)
{
	if (!m_bGrid)
	{
		int	iSectionIndex = GetSectionIndex(vPos);

		if (0 <= iSectionIndex && iSectionIndex < m_iSectionX * m_iSectionZ)
		{
			Vector3	vOrigin = vPos;
			vOrigin.y = m_vMax.y;
			Vector3	vDir = Vector3(0.f, -1.f, 0.f);

			for (int i = 0; i < m_pSection[iSectionIndex].tCellList.iSize; ++i)
			{
				Vector3	vIntersect;
				float	fDist = 0.f;
				PNavigationCell	pCell = m_pSection[iSectionIndex].tCellList.pCellList[i];

				if (RayIntersectTriangle(vOrigin, vDir, pCell->vPos[0], pCell->vPos[1], pCell->vPos[2], fDist, vIntersect))
					return pCell->iIndex;
			}
		}

		return -1;
	}

	Vector3	vCellSize = (m_vMax - m_vMin) / m_iLineRectCount;
	Vector3	vConvertPos = vPos - m_vMin;

	// ����, ���θ� 1�� ������ش�.
	vConvertPos.x /= vCellSize.x;
	vConvertPos.z /= vCellSize.z;

	// �簢�� �ε����� ���Ѵ�.
	int	idxX = (int)vConvertPos.x;
	int	idxZ = m_iLineRectCount - ((int)vConvertPos.z + 1);

	if (idxX < 0 || idxX > m_iLineRectCount)
		return -1;

	else if (idxZ < 0 || idxZ > m_iLineRectCount)
		return -1;

	return (idxZ * m_iLineRectCount + idxX) * 2;
}

bool CNavigationMesh::RayIntersectTriangle(Vector3 rayOrigin, Vector3 rayDir, Vector3 v0, Vector3 v1, Vector3 v2, float & t, Vector3 & vIntersect)
{
	Vector3 e1, e2, h, s, q;
	float a, f, u, v;

	e1 = v1 - v0;
	e2 = v2 - v0;
	h = rayDir.Cross(e2);
	a = e1.Dot(h);

	if (a > -0.00001 && a < 0.00001)
		return false;

	f = 1.f / a;
	s = rayOrigin - v0;

	u = f * s.Dot(h);

	if (u < 0.f || u > 1.f)
		return false;

	q = s.Cross(e1);

	v = f * rayDir.Dot(q);

	if (v < 0.f || u + v > 1.f)
		return false;

	t = f * e2.Dot(q);

	if (t > 0.00001)
	{
		vIntersect = rayOrigin + rayDir * t;
		return true;
	}

	return false;
}

int CNavigationMesh::GetSectionIndex(Vector3 vPos)
{
	vPos -= m_vMin;
	vPos /= m_vSectionSize;

	int x, z;
	x = (int)vPos.x;
	z = (int)vPos.z;

	return z * m_iSectionX + x;
}

void CNavigationMesh::Save(const char * pFileName, const string & strPathKey)
{
}

void CNavigationMesh::SaveFromFullPath(const char * pFullPath)
{
}

void CNavigationMesh::Load(const char * pFileName, const string & strPathKey)
{
}

void CNavigationMesh::LoadFromFullPath(const char * pFullPath)
{
}

bool CNavigationMesh::OpenListSort(const PNavigationCell & pSrc, const PNavigationCell & pDest)
{
	return pSrc->fTotal > pDest->fTotal;
}