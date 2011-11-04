#include <SFGUI/Scrollbar.hpp>
#include <SFGUI/Context.hpp>
#include <SFGUI/Engine.hpp>
#include <SFGUI/Engines/BREW.hpp>

namespace sfg {

Scrollbar::Scrollbar( Adjustment::Ptr adjustment, Orientation orientation ) :
	Range(),
	m_orientation( orientation ),
	m_dragging( false ),
	m_decrease_pressed( false ),
	m_increase_pressed( false ),
	m_page_decreasing( false ),
	m_page_increasing( false ),
	m_slider_click_offset( 0 )
{
	if( adjustment ) {
		SetAdjustment( adjustment );
	}
}

Scrollbar::Ptr Scrollbar::Create( Orientation orientation ) {
	Scrollbar::Ptr  ptr( new Scrollbar( Adjustment::Ptr(), orientation ) );
	return ptr;
}

Scrollbar::Ptr Scrollbar::Create( Adjustment::Ptr adjustment, Orientation orientation ) {
	Scrollbar::Ptr  ptr( new Scrollbar( adjustment, orientation ) );
	return ptr;
}

Scrollbar::Orientation Scrollbar::GetOrientation() const {
	return m_orientation;
}

const sf::FloatRect Scrollbar::GetSliderRect() const {
	float mimimum_slider_length( Context::Get().GetEngine().GetProperty<float>( "SliderMinimumLength", shared_from_this() ) );

	Adjustment::Ptr adjustment( GetAdjustment() );

	float current_value = adjustment->GetValue();
	float value_range = std::max( adjustment->GetUpper() - adjustment->GetLower() - adjustment->GetPageSize(), .0f );
	float pages = value_range / adjustment->GetPageSize() + 1.f;

	if( m_orientation == Horizontal ) {
		float stepper_length = GetAllocation().Height;
		float trough_length = GetAllocation().Width - 2.f * stepper_length;
		float slider_length = std::max( mimimum_slider_length, trough_length / pages );
		if( adjustment->GetPageSize() == .0f ) {
			slider_length = mimimum_slider_length;
		}

		float slider_x = stepper_length + ( trough_length - slider_length ) * current_value / value_range;
		float slider_y = 0.f;

		if( value_range == .0f ) {
			slider_x = stepper_length;
		}

		return sf::FloatRect( slider_x, slider_y, slider_length, GetAllocation().Height );
	}
	else {
		float stepper_length = GetAllocation().Width;
		float trough_length = GetAllocation().Height - 2.f * stepper_length;
		float slider_length = std::max( mimimum_slider_length, trough_length / pages );
		if( adjustment->GetPageSize() == .0f ) {
			slider_length = mimimum_slider_length;
		}

		float slider_x = 0.f;
		float slider_y = stepper_length + ( trough_length - slider_length ) * current_value / value_range;

		if( value_range == .0f ) {
			slider_y = stepper_length;
		}

		return sf::FloatRect( slider_x, slider_y, GetAllocation().Width, slider_length );
	}
}

bool Scrollbar::IsDecreaseStepperPressed() const {
	return m_decrease_pressed;
}

bool Scrollbar::IsIncreaseStepperPressed() const {
	return m_increase_pressed;
}


sf::Drawable* Scrollbar::InvalidateImpl() {
	return Context::Get().GetEngine().CreateScrollbarDrawable( std::dynamic_pointer_cast<Scrollbar>( shared_from_this() ) );
}

sf::Vector2f Scrollbar::GetRequisitionImpl() const {
	float mimimum_slider_length( Context::Get().GetEngine().GetProperty<float>( "SliderMinimumLength", shared_from_this() ) );

	// Scrollbars should always have a custom requisition set for it's shorter side.
	// If the dev forgets to set one show him where the scrollbar slider is so
	// it is easier for him to fix.
	return sf::Vector2f( mimimum_slider_length, mimimum_slider_length );
}

void Scrollbar::HandleMouseButtonEvent( sf::Mouse::Button button, bool press, int x, int y ) {
	if( button != sf::Mouse::Left ) {
		return;
	}

	if( press ) {
		sf::FloatRect slider_rect = GetSliderRect();
		slider_rect.Left += GetAllocation().Left;
		slider_rect.Top += GetAllocation().Top;

		if( slider_rect.Contains( static_cast<float>( x ), static_cast<float>( y ) ) ) {
			m_dragging = true;

			if( m_orientation == Horizontal ) {
				float slider_mid = slider_rect.Left + slider_rect.Width / 2.f;
				m_slider_click_offset = static_cast<float>( x ) - slider_mid;
			}
			else {
				float slider_mid = slider_rect.Top + slider_rect.Height / 2.f;
				m_slider_click_offset = static_cast<float>( y ) - slider_mid;
			}

			return;
		}

		if( m_orientation == Horizontal ) {
			float stepper_length = GetAllocation().Height;

			sf::FloatRect decrease_stepper_rect( GetAllocation().Left, GetAllocation().Top, stepper_length, GetAllocation().Height );
			sf::FloatRect increase_stepper_rect( GetAllocation().Left + GetAllocation().Width - stepper_length, GetAllocation().Top, stepper_length, GetAllocation().Height );

			if( decrease_stepper_rect.Contains( static_cast<float>( x ), static_cast<float>( y ) ) ) {
				m_decrease_pressed = true;
				GetAdjustment()->Decrement();
				m_change_timer.Reset();
				Invalidate();
				return;
			}

			if( increase_stepper_rect.Contains( static_cast<float>( x ), static_cast<float>( y ) ) ) {
				m_increase_pressed = true;
				GetAdjustment()->Increment();
				m_change_timer.Reset();
				Invalidate();
				return;
			}
		}
		else {
			float stepper_length = GetAllocation().Width;

			sf::FloatRect decrease_stepper_rect( GetAllocation().Left, GetAllocation().Top, GetAllocation().Width, stepper_length );
			sf::FloatRect increase_stepper_rect( GetAllocation().Left, GetAllocation().Top + GetAllocation().Height - stepper_length, GetAllocation().Width, stepper_length );

			if( decrease_stepper_rect.Contains( static_cast<float>( x ), static_cast<float>( y ) ) ) {
				m_decrease_pressed = true;
				GetAdjustment()->Decrement();
				m_change_timer.Reset();
				Invalidate();
				return;
			}

			if( increase_stepper_rect.Contains( static_cast<float>( x ), static_cast<float>( y ) ) ) {
				m_increase_pressed = true;
				GetAdjustment()->Increment();
				m_change_timer.Reset();
				Invalidate();
				return;
			}
		}

		float slider_center_x = slider_rect.Left + GetAbsolutePosition().x + slider_rect.Width / 2.f;
		float slider_center_y = slider_rect.Top + GetAbsolutePosition().y + slider_rect.Height / 2.f;

		if( m_orientation == Horizontal ) {
			if( GetAllocation().Contains( static_cast<float>( x ), static_cast<float>( y ) ) ) {
				if( static_cast<float>( x ) < slider_center_x ) {
					m_page_decreasing = true;
					m_page_decreasing = true;
					GetAdjustment()->DecrementPage();
					m_change_timer.Reset();
					Invalidate();
					return;
				}
				else {
					m_page_increasing = true;
					GetAdjustment()->IncrementPage();
					m_change_timer.Reset();
					Invalidate();
					return;
				}
			}
		}
		else {
			if( GetAllocation().Contains( static_cast<float>( x ), static_cast<float>( y ) ) ) {
				if( static_cast<float>( y ) < slider_center_y ) {
					m_page_decreasing = true;
					m_page_decreasing = true;
					GetAdjustment()->DecrementPage();
					m_change_timer.Reset();
					Invalidate();
					return;
				}
				else {
					m_page_increasing = true;
					GetAdjustment()->IncrementPage();
					m_change_timer.Reset();
					Invalidate();
					return;
				}
			}
		}
	}
	else {
		m_dragging = false;
		m_decrease_pressed = false;
		m_increase_pressed = false;
		m_page_decreasing = false;
		m_page_increasing = false;

		m_slider_click_offset = 0.f;

		Invalidate();
		return;
	}
}

void Scrollbar::HandleMouseMoveEvent( int x, int y ) {
	if( !m_dragging ) {
		return;
	}

	Adjustment::Ptr adjustment( GetAdjustment() );
	sf::FloatRect slider_rect = GetSliderRect();

	float value_range = std::max( adjustment->GetUpper() - adjustment->GetLower() - adjustment->GetPageSize(), adjustment->GetMinorStep() / 2.f );
	float steps = value_range / adjustment->GetMinorStep();

	if( m_orientation == Horizontal ) {
		float stepper_length = GetAllocation().Height;

		float slider_center_x = GetAbsolutePosition().x + slider_rect.Left + slider_rect.Width / 2.0f;
		float step_distance = ( GetAllocation().Width - 2.f * stepper_length ) / steps;

		float delta = static_cast<float>( x ) - ( slider_center_x + m_slider_click_offset );

		while( delta < ( -step_distance / 2 ) ) {
			adjustment->Decrement();
			delta += step_distance;
		}

		while( delta > ( step_distance / 2 ) ) {
			adjustment->Increment();
			delta -= step_distance;
		}
	}
	else {
		float stepper_length = GetAllocation().Width;

		float slider_center_y = GetAbsolutePosition().y + slider_rect.Top + slider_rect.Height / 2.0f;
		float step_distance = ( GetAllocation().Height - 2.f * stepper_length ) / steps;

		float delta =  static_cast<float>( y ) - ( slider_center_y + m_slider_click_offset );

		while( delta < ( -step_distance / 2 ) ) {
			adjustment->Decrement();
			delta += step_distance;
		}

		while( delta > ( step_distance / 2 ) ) {
			adjustment->Increment();
			delta -= step_distance;
		}
	}
}

void Scrollbar::HandleExpose( sf::RenderTarget& /*target*/ ) {
	float stepper_speed( Context::Get().GetEngine().GetProperty<float>( "Scrollbar.Stepper.Speed", shared_from_this() ) );

	// Increment / Decrement value while one of the steppers is pressed
	if( m_decrease_pressed && m_change_timer.GetElapsedTime() > static_cast<sf::Uint32>( 1000.f / stepper_speed ) ) {
		GetAdjustment()->Decrement();
		Invalidate();
		m_change_timer.Reset();
	}

	if( m_increase_pressed && m_change_timer.GetElapsedTime() > static_cast<sf::Uint32>( 1000.f / stepper_speed ) ) {
		GetAdjustment()->Increment();
		Invalidate();
		m_change_timer.Reset();
	}

	// Increment / Decrement page while mouse is pressed on the trough
	if( m_page_decreasing && m_change_timer.GetElapsedTime() > static_cast<sf::Uint32>( 1000.f / stepper_speed ) ) {
		GetAdjustment()->DecrementPage();
		Invalidate();
		m_change_timer.Reset();
	}

	if( m_page_increasing && m_change_timer.GetElapsedTime() > static_cast<sf::Uint32>( 1000.f / stepper_speed ) ) {
		GetAdjustment()->IncrementPage();
		Invalidate();
		m_change_timer.Reset();
	}
}

const std::string& Scrollbar::GetName() const {
	static const std::string name( "Scrollbar" );
	return name;
}

}
