/*
Copyright(c) 2016 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//================================
#include <QWidget>
#include <QGridLayout>
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "Core/GameObject.h"
#include <QDoubleValidator>
#include "DirectusCore.h"
#include "DirectusComboLabelText.h"
//=================================

class DirectusTransform : public QWidget
{
    Q_OBJECT
public:
    explicit DirectusTransform(QWidget *parent = 0);
    void Initialize(DirectusCore* directusCore);

    void Reflect(GameObject* gameobject);

    void SetPosition(Directus::Math::Vector3 pos);

    void SetRotation(Directus::Math::Quaternion rot);
    void SetRotation(Directus::Math::Vector3 rot);

    void SetScale(Directus::Math::Vector3 sca);

private:

    //= TITLE =======================
    QLabel* m_title;
    //===============================

    // = POSITION ===================
    QLabel* m_posLabel;
    DirectusComboLabelText* m_posX;
    DirectusComboLabelText* m_posY;
    DirectusComboLabelText* m_posZ;
    //===============================

    //= ROTATION ====================
    QLabel* m_rotLabel;
    DirectusComboLabelText* m_rotX;
    DirectusComboLabelText* m_rotY;
    DirectusComboLabelText* m_rotZ;
    //===============================

    //= SCALE =======================
    QLabel* m_scaLabel;
    DirectusComboLabelText* m_scaX;
    DirectusComboLabelText* m_scaY;
    DirectusComboLabelText* m_scaZ;
    //===============================

    //= LINE ========================
    QWidget* m_line;
    //===============================

    //= MISC ========================
    QGridLayout* m_gridLayout;
    QValidator* m_validator;
    //===============================

    Transform* m_inspectedTransform;
    DirectusCore* m_directusCore;

public slots:
    void MapPosition();
    void MapRotation();
    void MapScale();
};
