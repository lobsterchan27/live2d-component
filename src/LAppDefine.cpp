﻿/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppDefine.hpp"
#include <CubismFramework.hpp>

namespace LAppDefine {

    using namespace Csm;

    // 画面
    const csmFloat32 ViewScale = 1.0f;
    const csmFloat32 ViewMaxScale = 2.0f;
    const csmFloat32 ViewMinScale = 0.8f;

    const csmFloat32 ViewLogicalLeft = -1.0f;
    const csmFloat32 ViewLogicalRight = 1.0f;
    const csmFloat32 ViewLogicalBottom = -1.0f;
    const csmFloat32 ViewLogicalTop = -1.0f;

    const csmFloat32 ViewLogicalMaxLeft = -2.0f;
    const csmFloat32 ViewLogicalMaxRight = 2.0f;
    const csmFloat32 ViewLogicalMaxBottom = -2.0f;
    const csmFloat32 ViewLogicalMaxTop = 2.0f;

    // 相対パス
    csmChar* ResourcesPath = nullptr;

    // モデルの後ろにある背景の画像ファイル
    const csmChar* BackImageName = "back_class_normal.png";
    // 歯車
    const csmChar* GearImageName = "icon_gear.png";
    // 終了ボタン
    const csmChar* PowerImageName = "close.png";

    // モデル定義------------------------------------------
    // 外部定義ファイル(json)と合わせる
    const csmChar* MotionGroupIdle = "Idle"; // アイドリング
    const csmChar* MotionGroupTapBody = "TapBody"; // 体をタップしたとき
    const csmChar* MotionGroupReaction = "Reaction"; // リアクション
    const csmChar* MotionGroupNegativeReaction = "NegativeReaction"; // ネガティブリアクション

    // 外部定義ファイル(json)と合わせる
    const csmChar* HitAreaNameHead = "Head";
    const csmChar* HitAreaNameBody = "Body";

    // モーションの優先度定数
    const csmInt32 PriorityNone = 0;
    const csmInt32 PriorityIdle = 1;
    const csmInt32 PriorityNormal = 2;
    const csmInt32 PriorityForce = 3;

    // MOC3の整合性検証オプション
    const csmBool MocConsistencyValidationEnable = true;

    // デバッグ用ログの表示オプション
    const csmBool DebugLogEnable = true;
    const csmBool DebugTouchLogEnable = false;

    // Frameworkから出力するログのレベル設定
    const CubismFramework::Option::LogLevel CubismLoggingLevel = CubismFramework::Option::LogLevel_Verbose;

    // デフォルトのレンダーターゲットサイズ
    csmInt32 RenderTargetWidth = 1080;
    csmInt32 RenderTargetHeight = 1920;
    csmFloat32 AudioDuration = 0.0f;
    csmFloat32 VideoVDuration = 0.0f;
    csmFloat32 VideoADuration = 0.0f;
    double VideoFPS = 0.0f;
    csmFloat32 postDuration = 5.0f;

    csmFloat32 deltaTime = 0.0f;
}