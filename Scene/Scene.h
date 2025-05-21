//=============================================================================
//
// シーン全体をしっかり仕切る舞台監督ちゃん [Scene.h]
// Author :
// 描画対象のGameObjectを登録・管理して、影やエフェクトにも必要な情報を提供してくれる頼れる子ですっ！
// ゲーム内のすべての主役たちが、彼女のリストから登場するの
// 
//=============================================================================
#pragma once

#include "main.h"
#include "Utility/SingletonBase.h"

class IGameObject;

class Scene : public SingletonBase<Scene>
{
public:

    // 描画対象のGameObjectを登録
    void RegisterGameObject(IGameObject* obj)
    {
        m_renderableObjects.push_back(obj);
    }

    // 描画対象のGameObjectを削除
    void UnregisterGameObject(IGameObject* obj)
    {
        int index = m_renderableObjects.find_index(obj);
        if (index != -1)
            m_renderableObjects.erase(index);
    }

    //描画対象すべてを取得（ShadowRenderer等から使用）
    const SimpleArray<IGameObject*>& GetAllRenderableObjects() const
    {
        return m_renderableObjects;
    }

private:

    SimpleArray<IGameObject*> m_renderableObjects;
};