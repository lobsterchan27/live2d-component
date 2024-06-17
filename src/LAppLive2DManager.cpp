/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppLive2DManager.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Rendering/CubismRenderer.hpp>
#include "LAppPal.hpp"
#include "LAppDefine.hpp"
#include "LAppDelegate.hpp"
#include "LAppModel.hpp"
#include "LAppView.hpp"
#include <iostream>

#include "FloatingPlatform.hpp"

using namespace Csm;
using namespace LAppDefine;
using namespace std;

namespace
{
    LAppLive2DManager *s_instance = NULL;

    void FinishedMotion(ACubismMotion *self)
    {
        LAppPal::PrintLogLn("Motion Finished: %x", self);
    }

    int CompareCsmString(const void *a, const void *b)
    {
        return strcmp(reinterpret_cast<const Csm::csmString *>(a)->GetRawString(),
                      reinterpret_cast<const Csm::csmString *>(b)->GetRawString());
    }

    float getRandomFloat(float min, float max)
    {
        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }
}

LAppLive2DManager *LAppLive2DManager::GetInstance()
{
    if (s_instance == NULL)
    {
        s_instance = new LAppLive2DManager();
    }

    return s_instance;
}

void LAppLive2DManager::ReleaseInstance()
{
    if (s_instance != NULL)
    {
        delete s_instance;
    }

    s_instance = NULL;
}

LAppLive2DManager::LAppLive2DManager()
    : _viewMatrix(NULL),
      _sceneIndex(0),
      _isTalking(false),
      _talkDuration(0.0f),
      _silenceDuration(0.0f),
      _potentialSilenceDuration(0.0f),
      _silenceThreshold(0.8f),
      _lastMotionTime(0.0f),
      _isMotion(false)
{
    _viewMatrix = new CubismMatrix44();
    SetUpModel();

    ChangeScene(_sceneIndex);
    _elapsedTime = FloatingPlatform::getInstance()->getElapsedSeconds();
}

LAppLive2DManager::~LAppLive2DManager()
{
    ReleaseAllModel();
    delete _viewMatrix;
}

void LAppLive2DManager::SetFinishedMotionTime()
{
    _finishedMotionTime = *_elapsedTime;
}

void LAppLive2DManager::UpdateTalkAndSilenceDuration()
{
    FloatingPlatform *platform = FloatingPlatform::getInstance();
    LAppWavFileHandler *handler = platform->getWavFileHandler();
    csmFloat32 rms = handler->GetRms(true);
    if (rms > 0.00f)
    {
        _isTalking = true;
        _talkDuration += deltaTime;
        _silenceDuration = 0.0f;          // Reset silence duration
        _potentialSilenceDuration = 0.0f; // Reset potential silence duration
    }

    if (_isTalking)
    {
        _potentialSilenceDuration += deltaTime;
        if (_potentialSilenceDuration >= _silenceThreshold)
        {
            _isTalking = false;
            _talkDuration = 0.0f;
            _silenceDuration = _potentialSilenceDuration;
            _potentialSilenceDuration = 0.0f;
        }
    }
    else
    {
        _silenceDuration += deltaTime;
    }

    // if (_talkDuration > 0.0f)
    // {
    //     cout << "Talk Duration: " << _talkDuration << " seconds" << endl;
    // }
    // else if (_silenceDuration > 0.0f)
    // {
    //     cout << "Silence Duration: " << _silenceDuration << " seconds" << endl;
    // }
}

void LAppLive2DManager::ReleaseAllModel()
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        delete _models[i];
    }

    _models.Clear();
}

void LAppLive2DManager::SetUpModel()
{
    // ResourcesPathの中にあるフォルダ名を全てクロールし、モデルが存在するフォルダを定義する。
    // フォルダはあるが同名の.model3.jsonが見つからなかった場合はリストに含めない。
    csmString crawlPath(ResourcesPath);
    crawlPath += "*.*";

    struct _finddata_t fdata;
    intptr_t fh = _findfirst(crawlPath.GetRawString(), &fdata);
    if (fh == -1)
        return;

    _modelDir.Clear();

    while (_findnext(fh, &fdata) == 0)
    {
        if ((fdata.attrib & _A_SUBDIR) && strcmp(fdata.name, "..") != 0)
        {
            // フォルダと同名の.model3.jsonがあるか探索する
            csmString model3jsonPath(ResourcesPath);
            model3jsonPath += fdata.name;
            model3jsonPath.Append(1, '/');
            model3jsonPath += fdata.name;
            model3jsonPath += ".model3.json";

            struct _finddata_t fdata2;
            if (_findfirst(model3jsonPath.GetRawString(), &fdata2) != -1)
            {
                _modelDir.PushBack(csmString(fdata.name));
            }
        }
    }
    qsort(_modelDir.GetPtr(), _modelDir.GetSize(), sizeof(csmString), CompareCsmString);
}

csmVector<csmString> LAppLive2DManager::GetModelDir() const
{
    return _modelDir;
}

csmInt32 LAppLive2DManager::GetModelDirSize() const
{
    return _modelDir.GetSize();
}

LAppModel *LAppLive2DManager::GetModel(csmUint32 no) const
{
    if (no < _models.GetSize())
    {
        return _models[no];
    }

    return NULL;
}

void LAppLive2DManager::OnDrag(csmFloat32 x, csmFloat32 y) const
{
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        LAppModel *model = GetModel(i);

        model->SetDragging(x, y);
    }
}

void LAppLive2DManager::OnTap(csmFloat32 x, csmFloat32 y) const
{
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]tap point: {x:%.2f y:%.2f}", x, y);
    }

    for (csmUint32 i = 0; i < _models.GetSize(); i++)
    {
        if (_models[i]->HitTest(HitAreaNameHead, x, y))
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("[APP]hit area: [%s]", HitAreaNameHead);
            }
            _models[i]->SetRandomExpression();
        }
        else if (_models[i]->HitTest(HitAreaNameBody, x, y))
        {
            if (DebugLogEnable)
            {
                LAppPal::PrintLogLn("[APP]hit area: [%s]", HitAreaNameBody);
            }
            _models[i]->StartRandomMotion(MotionGroupTapBody, PriorityNormal, FinishedMotion);
        }
    }
}

void LAppLive2DManager::FinishedMotionStatic(ACubismMotion *self)
{
    LAppLive2DManager *manager = LAppLive2DManager::GetInstance();
    manager->UpdateMotionTime();
    manager->_isMotion = false;
    LAppPal::PrintLogLn("Motion Finished: %x", self);
}

void LAppLive2DManager::UpdateMotionTime()
{
    _lastMotionTime = *_elapsedTime;
}

void LAppLive2DManager::OnUpdate()
{
    int width, height;
    float modelScale, correctionValueX, correctionValueY;
    // glfwGetWindowSize(LAppDelegate::GetInstance()->GetWindow(), &width, &height);
    width = RenderTargetWidth;
    height = RenderTargetHeight;
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    csmUint32 modelCount = _models.GetSize();
    for (csmUint32 i = 0; i < modelCount; ++i)
    {
        // _models[i]->SetRandomExpression();
        CubismMatrix44 projection;
        LAppModel *model = GetModel(i);
        CubismModel *csmModel = model->GetModel();
        CubismModelMatrix *modelMatrix = model->GetModelMatrix();

        if (model->GetModel() == NULL)
        {
            LAppPal::PrintLogLn("Failed to model->GetModel().");
            continue;
        }

        if (model->GetModel()->GetCanvasWidth() > 1.0f && width < height)
        {
            // 横に長いモデルを縦長ウィンドウに表示する際モデルの横サイズでscaleを算出する
            model->GetModelMatrix()->SetWidth(2.0f);
            projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height));
        }
        else
        {
            projection.Scale(static_cast<float>(height) / static_cast<float>(width), 1.0f);
        }

        // 必要があればここで乗算
        if (_viewMatrix != NULL)
        {
            projection.MultiplyByMatrix(_viewMatrix);
        }

        // // モデル1体描画前コール
        // LAppDelegate::GetInstance()->GetView()->PreModelDraw(*model);

        // #lobby translation matrix/projection matrix
        // TODO: REFACTOR TO NOT BE DOGTRASH
        float offsetX, offsetY;
        float m = 0.822857f;
        float c = 1.03714f;
        float maxScale = 3.0f;
        float minScale = 1.0f;

        // Model scale calculation
        modelScale = m * aspectRatio + c;

        if (modelScale > maxScale)
        {
            modelScale = maxScale;
        }
        else if (modelScale < minScale)
        {
            modelScale = minScale;
        }

        modelMatrix->SetWidth(modelScale);
        correctionValueX = projection.GetScaleX() * modelScale;
        correctionValueY = projection.GetScaleY() * modelScale;

        // dividing by 2 to get half the width
        offsetY = correctionValueY / 2 * 0.64f;
        offsetX = correctionValueX / 2;
        offsetX = offsetX - (offsetX * 0.64f); // White Space offsetting.

        projection.TranslateY(-1.0f - offsetY);
        projection.TranslateX(1.0f - offsetX);

        UpdateTalkAndSilenceDuration();
        float randomInterval = getRandomFloat(1.0f, 5.0f);
        if (*_elapsedTime - _lastMotionTime > randomInterval && !_isMotion)
        {
            _isMotion = true;
            model->StartRandomMotion(MotionGroupReaction, PriorityNormal, FinishedMotionStatic);
        }

        model->Update();
        model->Draw(projection); ///< 参照渡しなのでprojectionは変質する

        // モデル1体描画後コール
        // LAppDelegate::GetInstance()->GetView()->PostModelDraw(*model);
    }
}

void LAppLive2DManager::NextScene()
{
    csmInt32 no = (_sceneIndex + 1) % GetModelDirSize();
    ChangeScene(no);
}

void LAppLive2DManager::ChangeScene(Csm::csmInt32 index)
{
    _sceneIndex = index;
    if (DebugLogEnable)
    {
        LAppPal::PrintLogLn("[APP]model index: %d", _sceneIndex);
    }

    // model3.jsonのパスを決定する.
    // ディレクトリ名とmodel3.jsonの名前を一致していることが条件
    const csmString &model = _modelDir[index];

    csmString modelPath(ResourcesPath);
    modelPath += model;
    modelPath.Append(1, '/');

    csmString modelJsonName(model);
    modelJsonName += ".model3.json";

    ReleaseAllModel();
    _models.PushBack(new LAppModel());
    _models[0]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());

    LAppWavFileHandler *handler = FloatingPlatform::getInstance()->getWavFileHandler();
    _models[0]->setWavFileHandler(handler);

    /*
     * モデル半透明表示を行うサンプルを提示する。
     * ここでUSE_RENDER_TARGET、USE_MODEL_RENDER_TARGETが定義されている場合
     * 別のレンダリングターゲットにモデルを描画し、描画結果をテクスチャとして別のスプライトに張り付ける。
     */
    {
#if defined(USE_RENDER_TARGET)
        // LAppViewの持つターゲットに描画を行う場合、こちらを選択
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ViewFrameBuffer;
#elif defined(USE_MODEL_RENDER_TARGET)
        // 各LAppModelの持つターゲットに描画を行う場合、こちらを選択
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_ModelFrameBuffer;
#else
        // デフォルトのメインフレームバッファへレンダリングする(通常)
        LAppView::SelectTarget useRenderTarget = LAppView::SelectTarget_None;
#endif

#if defined(USE_RENDER_TARGET) || defined(USE_MODEL_RENDER_TARGET)
        // モデル個別にαを付けるサンプルとして、もう1体モデルを作成し、少し位置をずらす
        _models.PushBack(new LAppModel());
        _models[1]->LoadAssets(modelPath.GetRawString(), modelJsonName.GetRawString());
        _models[1]->GetModelMatrix()->TranslateX(0.2f);
#endif

        LAppDelegate::GetInstance()->GetView()->SwitchRenderingTarget(useRenderTarget);

        // 別レンダリング先を選択した際の背景クリア色
        float clearColor[3] = {1.0f, 1.0f, 1.0f};
        LAppDelegate::GetInstance()->GetView()->SetRenderTargetClearColor(clearColor[0], clearColor[1], clearColor[2]);
    }
}

csmUint32 LAppLive2DManager::GetModelNum() const
{
    return _models.GetSize();
}

void LAppLive2DManager::SetViewMatrix(CubismMatrix44 *m)
{
    for (int i = 0; i < 16; i++)
    {
        _viewMatrix->GetArray()[i] = m->GetArray()[i];
    }
}
