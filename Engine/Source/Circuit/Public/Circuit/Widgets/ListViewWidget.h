#pragma once

#include "Circuit/Widgets/CompoundWidget.h"
#include "Circuit/Widgets/BorderWidget.h"
#include "Circuit/Widgets/TextWidget.h"
#include "Circuit/Widgets/ScrollBoxWidget.h"

#include "Circuit/CircuitPainter.h"
#include "Circuit/WidgetInteractionData.h"

#include <CoreUtilities/Containers/Vector.h>

#include <InputModule/InputCodes.h>

#include <unordered_set>

namespace Circuit
{
	DECLARE_DELEGATE_OneParam(OnListRowInteraction, const WidgetInteractionData&);
	template<typename ItemType>
	class IListViewRow : public CompoundWidget
	{
	public:
		virtual ~IListViewRow() = default;

		CIRCUIT_BEGIN_ARGS(IListViewRow)
			:_Content(nullptr)

		{};

		CIRCUIT_ARGUMENT(Ref<Widget>, Content);

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args)
		{
			if (args._Content)
			{
				Ref<BorderWidget> border = CreateWidget(BorderWidget)
					.Padding(2.f)
					.Content(args._Content)
					.BackgroundColor_Raw(this, &IListViewRow<ItemType>::GetBackgroundColor);
				AddChildWidget(border);
			}
		}

		virtual void OnBeginHover(const WidgetInteractionData& interactionData) override
		{
			m_isHovered = true;
		}
		virtual void OnEndHover(const WidgetInteractionData& interactionData) override
		{
			m_isHovered = false;
			m_isPressed = false;
		}

		virtual void OnPressed(const WidgetInteractionData& interactionData) override
		{
			m_isPressed = true;
			m_onPressedRow.ExecuteIfBound(interactionData);
		}
		virtual void OnReleased(const WidgetInteractionData& interactionData) override
		{
			m_isPressed = false;
			m_onReleasedRow.ExecuteIfBound(interactionData);
		}
		virtual void OnDoubleClicked(const WidgetInteractionData& interactionData) override
		{
			m_onDoubleClickRow.ExecuteIfBound(interactionData);
		}


		void SetColorAttribute(Volt::Attribute<CircuitColor> color)
		{
			m_color = color;
		}
		void SetHoveredColorAttribute(Volt::Attribute<CircuitColor> hoveredColor)
		{
			m_hoveredColor = hoveredColor;
		}
		void SetPressedColorAttribute(Volt::Attribute<CircuitColor> pressedColor)
		{
			m_pressedColor = pressedColor;
		}

		OnListRowInteraction& GetOnRowPressedDelegate()
		{
			return m_onPressedRow;
		}
		OnListRowInteraction& GetOnRowDoubleClickedDelegate()
		{
			return m_onDoubleClickRow;
		}
		OnListRowInteraction& GetOnRowReleasedDelegate()
		{
			return m_onReleasedRow;
		}

	private:
		CircuitColor GetBackgroundColor() const
		{
			if (m_isPressed)
			{
				return m_pressedColor.Get();
			}

			if (m_isHovered)
			{
				return m_hoveredColor.Get();
			}

			return m_color.Get();
		}

		OnListRowInteraction m_onDoubleClickRow;
		OnListRowInteraction m_onPressedRow;
		OnListRowInteraction m_onReleasedRow;

		Volt::Attribute<CircuitColor> m_color = CircuitColor(0x555555ff);
		Volt::Attribute<CircuitColor> m_hoveredColor = CircuitColor(0xaa5555ff);
		Volt::Attribute<CircuitColor> m_pressedColor = CircuitColor(0x5555aaff);

		bool m_isHovered = false;
		bool m_isPressed = false;
	};

	template<typename ItemType>
	class ListViewWidget : public CompoundWidget
	{
	public:
		DECLARE_DELEGATE_RetVal_OneParam(Ref<IListViewRow<ItemType>>, OnGenerateRowDelegate, ItemType&);
		DECLARE_DELEGATE_OneParam(OnRowInteractDelegate, ItemType&);

		CIRCUIT_BEGIN_ARGS(ListViewWidget)
			: _ItemsSource(nullptr)
		{};

		CIRCUIT_ARGUMENT(Vector<ItemType>*, ItemsSource);
		CIRCUIT_EVENT(OnGenerateRowDelegate, OnGenerateRow);

		CIRCUIT_EVENT(OnRowInteractDelegate, OnRowDoubleClicked);

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args)
		{
			m_itemsSource = args._ItemsSource;
			m_onGenerateRow = args._OnGenerateRow;
			m_onRowDoubleClicked = args._OnRowDoubleClicked;

			m_scrollBox = CreateWidget(Circuit::ScrollBoxWidget)
				.AllowVerticalScroll(true)
				.AllowHorizontalScroll(false)
				.BackgroundColor(CircuitColor(40, 40, 40))
				.ScrollBarTrackColor(CircuitColor(50, 50, 50))
				.ScrollBarThumbColor(CircuitColor(100, 100, 100))
				.ScrollBarThumbHoverColor(CircuitColor(140, 140, 140))
				.ContentSize_Lambda([this]() 
			{
				return GetDesiredSize();
			});

			AddChildWidget(m_scrollBox);

			RegenerateRows();
		}

		void SetItemsSource(Vector<ItemType>* itemsSource)
		{
			m_itemsSource = itemsSource;
			RegenerateRows();
		}

		virtual glm::vec2 GetDesiredSize() override
		{
			float totalHeight = 0.f;
			for (size_t i = 0; i < m_rowWidgets.size(); i++)
			{
				if (m_rowWidgets[i])
				{
					const glm::vec2 desiredSize = m_rowWidgets[i]->GetDesiredSize();
					totalHeight += desiredSize.y > 0.f ? desiredSize.y : s_defaultRowHeight;
				}
			}
			return { -1, totalHeight > 0.f ? totalHeight : -1 };
		}

		virtual void OnPaint(CircuitPainter& painter) override
		{
			if (!m_itemsSource || !m_onGenerateRow.IsBound())
			{
				return;
			}

			if (m_rowWidgets.size() != m_itemsSource->size())
			{
				RegenerateRows();
			}

			const glm::vec2 allottedSize = painter.GetAllottedSize();
			painter.AddWidget(m_scrollBox, 0, 0, allottedSize.x, allottedSize.y);

			float currentOffset = m_scrollBox->GetHorizontalScrollOffset();
			for (size_t i = 0; i < m_rowWidgets.size(); i++)
			{
				if (m_rowWidgets[i])
				{
					const glm::vec2 desiredSize = m_rowWidgets[i]->GetDesiredSize();
					const float height = desiredSize.y > 0.f ? desiredSize.y : s_defaultRowHeight;

					painter.AddWidget(m_rowWidgets[i], 0.f, currentOffset, allottedSize.x, height);
					currentOffset += height;
				}
			}
		}

		virtual bool IsHittestInvisible() const override { return true; }

		void RegenerateRows()
		{
			m_rowWidgets.clear();
			ClearChildWidgets();

			if (!m_itemsSource || !m_onGenerateRow.IsBound())
			{
				return;
			}

			for (size_t i = 0; i < m_itemsSource->size(); i++)
			{
				Ref<IListViewRow<ItemType>> rowWidget = m_onGenerateRow.Execute((*m_itemsSource)[i]);

				if (rowWidget)
				{
					Volt::Attribute<CircuitColor> colorAttribute;
					colorAttribute.BindRaw(this, &ListViewWidget::GetRowColor, rowWidget);
					rowWidget->SetColorAttribute(colorAttribute);

					Volt::Attribute<CircuitColor> hoveredColorAttribute;
					hoveredColorAttribute.BindRaw(this, &ListViewWidget::GetRowHoveredColor, rowWidget);
					rowWidget->SetHoveredColorAttribute(hoveredColorAttribute);

					Volt::Attribute<CircuitColor> pressedColorAttribute;
					pressedColorAttribute.BindRaw(this, &ListViewWidget::GetRowPressedColor, rowWidget);
					rowWidget->SetPressedColorAttribute(pressedColorAttribute);

					rowWidget->GetOnRowReleasedDelegate().BindRaw(this, &ListViewWidget::RowClicked, rowWidget);
					rowWidget->GetOnRowDoubleClickedDelegate().BindRaw(this, &ListViewWidget::RowDoubleClicked, &(*m_itemsSource)[i]);

					m_rowWidgets.push_back(rowWidget);
					AddChildWidget(rowWidget);
				}
			}
		}

		CircuitColor GetRowColor(Ref<IListViewRow<ItemType>> row) const
		{
			if (m_selectedRows.contains(row))
			{
				return 0x556666ff;
			}

			return 0x555555ff;
		}

		CircuitColor GetRowHoveredColor(Ref<IListViewRow<ItemType>> row) const
		{
			if (m_selectedRows.contains(row))
			{
				return 0x667777ff;
			}

			return 0x666666ff;
		}

		CircuitColor GetRowPressedColor(Ref<IListViewRow<ItemType>> row) const
		{
			if (m_selectedRows.contains(row))
			{
				return 0x445555ff;
			}

			return 0x444444ff;
		}

	private:
		void RowClicked(const WidgetInteractionData& interactionData, Ref<IListViewRow<ItemType>> row)
		{
			if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
			{
				return;
			}

			if (!m_selectedRows.contains(row))
			{
				m_selectedRows.clear();
				m_selectedRows.insert(row);
			}
		}

		void RowDoubleClicked(const WidgetInteractionData& interactionData, ItemType* item)
		{
			if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
			{
				return;
			}

			m_onRowDoubleClicked.ExecuteIfBound(*item);
		}

		std::unordered_set<Ref<IListViewRow<ItemType>>> m_selectedRows;
		Vector<ItemType>* m_itemsSource = nullptr;
		OnGenerateRowDelegate m_onGenerateRow;
		OnRowInteractDelegate m_onRowDoubleClicked;
		Vector<Ref<IListViewRow<ItemType>>> m_rowWidgets;
		Ref<ScrollBoxWidget> m_scrollBox;

		static constexpr float s_defaultRowHeight = 24.f;
	};
}
