// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NavigationViewItemsFactory.h"
#include "NavigationViewItemBase.h"
#include "NavigationViewItem.h"
#include "ItemTemplateWrapper.h"
#include <ElementFactoryRecycleArgs.h>

void NavigationViewItemsFactory::UserElementFactory(winrt::IInspectable const& newValue)
{
    m_itemTemplateWrapper = newValue.try_as<winrt::IElementFactoryShim>();
    if (!m_itemTemplateWrapper)
    {
        // ItemTemplate set does not implement IElementFactoryShim. We also 
        // want to support DataTemplate and DataTemplateSelectors automagically.
        if (auto dataTemplate = newValue.try_as<winrt::DataTemplate>())
        {
            m_itemTemplateWrapper = winrt::make<ItemTemplateWrapper>(dataTemplate);
        }
        else if (auto selector = newValue.try_as<winrt::DataTemplateSelector>())
        {
            m_itemTemplateWrapper = winrt::make<ItemTemplateWrapper>(selector);
        }
    }
}

winrt::UIElement NavigationViewItemsFactory::GetElementCore(winrt::ElementFactoryGetArgs const& args)
{
    auto const newContent = [itemTemplateWrapper = m_itemTemplateWrapper, args]() {
        if (itemTemplateWrapper)
        {
            return itemTemplateWrapper.GetElement(args).as<winrt::IInspectable>();
        }
        return args.Data();
    }();

    if (auto const newItem = newContent.try_as<winrt::NavigationViewItemBase>())
    {
        return newItem;
    }

    // Create a wrapping container for the data
    auto nvi = winrt::make_self<NavigationViewItem>();
    nvi->Content(newContent);
    if (newContent != args.Data())
    {
        if (auto itemTemplateWrapper = m_itemTemplateWrapper.try_as<ItemTemplateWrapper>())
        {
            if (auto dataTemplate = itemTemplateWrapper->Template())
            {
                nvi->ContentTemplate(dataTemplate);
            }
            else if (auto selector = itemTemplateWrapper->TemplateSelector())
            {
                nvi->ContentTemplateSelector(selector);
            }
            nvi->Content(args.Data());

            // Recycle newContent
            auto const args = winrt::make_self<ElementFactoryRecycleArgs>();
            args->Element(newContent.try_as<winrt::UIElement>());
            m_itemTemplateWrapper.RecycleElement(static_cast<winrt::ElementFactoryRecycleArgs>(*args));
        }
    }
    return *nvi;
}

void NavigationViewItemsFactory::RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args)
{
    if (m_itemTemplateWrapper)
    {
        m_itemTemplateWrapper.RecycleElement(args);
    }
    else
    {
        // We want to unlink the containers from the parent repeater
        // in case we are required to move it to a different repeater.
        if (auto panel = args.Parent().try_as<winrt::Panel>())
        {
            auto children = panel.Children();
            unsigned int childIndex = 0;
            if (children.IndexOf(args.Element(), childIndex))
            {
                children.RemoveAt(childIndex);
            }
        }
    }
}
