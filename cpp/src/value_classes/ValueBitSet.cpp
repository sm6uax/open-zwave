//-----------------------------------------------------------------------------
//
//	ValueBitSet.cpp
//
//	Represents a boolean value
//
//	Copyright (c) 2017 Justin Hammond <justin@dynam.ac>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "tinyxml.h"
#include "value_classes/ValueBitSet.h"
#include "Driver.h"
#include "Node.h"
#include "platform/Log.h"
#include "Manager.h"
#include <ctime>

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <ValueBitSet::ValueBitSet>
// Constructor
//-----------------------------------------------------------------------------
ValueBitSet::ValueBitSet
(
	uint32 const _homeId,
	uint8 const _nodeId,
	ValueID::ValueGenre const _genre,
	uint8 const _commandClassId,
	uint8 const _instance,
	uint16 const _index,
	string const& _label,
	string const& _units,
	bool const _readOnly,
	bool const _writeOnly,
	uint32 const _value,
	uint8 const _pollIntensity
):
  	Value( _homeId, _nodeId, _genre, _commandClassId, _instance, _index, ValueID::ValueType_Bool, _label, _units, _readOnly, _writeOnly, false, _pollIntensity ),
	m_value( _value ),
	m_valueCheck( false ),
	m_newValue( false ),
	m_BitMask(0xFFFFFFFF),
	m_size(0)
{
}

bool ValueBitSet::SetFromString
(
	string const& _value
)
{
	int32 val = atoi( _value.c_str() );
	return Set( val );
}

string const ValueBitSet::GetAsString
(
) const
{
	stringstream ss;
	ss << GetValue();
	return ss.str();
}

string const ValueBitSet::GetAsBinaryString
(
) const
{
	uint32 n = GetValue();
	std::string r;
    while(n!=0) {r=(n%2==0 ?"0":"1")+r; n/=2;}
    return "0b" + r;
}

uint32 ValueBitSet::GetValue
(
) const
{
	return m_value.GetValue();
}
bool ValueBitSet::GetBit
(
		uint8 _idx
) const
{
	if (isValidBit(_idx))
		return m_value.IsSet(_idx);
	Log::Write(LogLevel_Warning, m_id.GetNodeId(), "GetBit Index %d is not valid with BitMask %d", _idx, m_BitMask);
	return false;

}



//-----------------------------------------------------------------------------
// <ValueBitSet::ReadXML>
// Apply settings from XML
//-----------------------------------------------------------------------------
void ValueBitSet::ReadXML
(
	uint32 const _homeId,
	uint8 const _nodeId,
	uint8 const _commandClassId,
	TiXmlElement const* _valueElement
)
{
	Value::ReadXML( _homeId, _nodeId, _commandClassId, _valueElement );

	int intVal;
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "bitmask", &intVal ) )
	{
		m_BitMask = (uint32)intVal;
	}
	else
	{
		Log::Write( LogLevel_Info, "Missing BitMask value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
	}
	if( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "value", &intVal ) )
	{
		m_value.SetValue((uint32)intVal);
	}
	else
	{
		Log::Write( LogLevel_Info, "Missing default integer value from xml configuration: node %d, class 0x%02x, instance %d, index %d", _nodeId,  _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
	}
	// Get size of values
	int intSize;
	if ( TIXML_SUCCESS == _valueElement->QueryIntAttribute( "size", &intSize ) )
	{
		if( intSize == 1 || intSize == 2 || intSize == 4 )
		{
			m_size = intSize;
		}
		else
		{
			Log::Write( LogLevel_Info, "Value size is invalid. Only 1, 2 & 4 supported for node %d, class 0x%02x, instance %d, index %d", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
			m_size = 1;
		}
	}
	else
	{
		Log::Write( LogLevel_Info, "Value list size is not set, assuming 1 bytes for node %d, class 0x%02x, instance %d, index %d", _nodeId, _commandClassId, GetID().GetInstance(), GetID().GetIndex() );
		m_size = 1;
	}
	TiXmlElement const *helpElement = _valueElement->FirstChildElement("Help");
	TiXmlElement const *BitSetHelpElement = helpElement->FirstChildElement("BitSet");
	while (BitSetHelpElement) {
		uint8 id;
		if( TIXML_SUCCESS == BitSetHelpElement->QueryIntAttribute( "id", &intVal ) )
			{
				id = (uint8)intVal;
			}
		char const* label = _valueElement->Attribute( "label" );
		if( label )
		{
			m_BitLabelString[id] = label;
		}
		string helpstring = BitSetHelpElement->GetText();
		m_BitHelpString[id] = helpstring;
		BitSetHelpElement = BitSetHelpElement->NextSiblingElement("BitSet");
	}

}

//-----------------------------------------------------------------------------
// <ValueBitSet::WriteXML>
// Write ourselves to an XML document
//-----------------------------------------------------------------------------
void ValueBitSet::WriteXML
(
	TiXmlElement* _valueElement
)
{
	Value::WriteXML( _valueElement );
	char str[16];

	snprintf(str, sizeof(str), "%d", m_BitMask);
	_valueElement->SetAttribute( "bitmask", str);

	snprintf( str, sizeof(str), "%d", m_value.GetValue() );
	_valueElement->SetAttribute( "value", str );

	snprintf( str, sizeof(str), "%d", m_size );
	_valueElement->SetAttribute( "size", str );

	TiXmlElement *helpElement = _valueElement->FirstChildElement("Help");
	for (std::map<uint8, string>::iterator it = m_BitHelpString.begin(); it != m_BitHelpString.end(); ++it) {
		TiXmlElement* BitSethelpElement = new TiXmlElement( "BitSet" );
		BitSethelpElement->SetAttribute("id", it->first);
		BitSethelpElement->SetAttribute("label", m_BitLabelString.at(it->first).c_str());
		TiXmlText* textElement = new TiXmlText(it->second.c_str());
		BitSethelpElement->LinkEndChild( textElement );
		helpElement->LinkEndChild( BitSethelpElement );
	}
}

//-----------------------------------------------------------------------------
// <ValueBitSet::Set>
// Set a new value in the device
//-----------------------------------------------------------------------------
bool ValueBitSet::Set
(
	uint32 const _value
)
{
	if (_value & ~ m_BitMask) {
		Log::Write(LogLevel_Warning, m_id.GetNodeId(), "Set: Value %d is not valid with BitMask %d", _value, m_BitMask);
		return false;
	}


	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueBitSet* tempValue = new ValueBitSet( *this );
	tempValue->m_value.SetValue(_value);

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}

bool ValueBitSet::SetBit
(
		uint8 const _idx
)
{
	/* is the bits valid */
	if (!isValidBit(_idx)) {
		std::cout << "notok" << std::endl;
		Log::Write(LogLevel_Warning, m_id.GetNodeId(), "SetBit: Bit %d is not valid with BitMask %d", _idx, m_BitMask);
		return false;
	}


	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueBitSet* tempValue = new ValueBitSet( *this );
	tempValue->m_value.Set(_idx);

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}
bool ValueBitSet::ClearBit
(
		uint8 const _idx
)
{

	/* is the bits valid */
	if (!isValidBit(_idx)) {
		Log::Write(LogLevel_Warning, m_id.GetNodeId(), "ClearBit: Bit %d is not valid with BitMask %d", _idx, m_BitMask);
		return false;
	}

	// create a temporary copy of this value to be submitted to the Set() call and set its value to the function param
  	ValueBitSet* tempValue = new ValueBitSet( *this );
	tempValue->m_value.Clear(_idx);

	// Set the value in the device.
	bool ret = ((Value*)tempValue)->Set();

	// clean up the temporary value
	delete tempValue;

	return ret;
}

bool ValueBitSet::SetBitMask
(
		uint32 _bitMask
)
{
	m_BitMask = _bitMask;
	return true;
}

uint32 ValueBitSet::GetBitMask
(
) const
{
	return m_BitMask;
}

string ValueBitSet::GetBitHelp
(
		uint8 _idx
)
{
	if (isValidBit(_idx)) {
		return m_BitHelpString.at(_idx);
	}
	Log::Write(LogLevel_Warning, m_id.GetNodeId(), "SetBitHelp: Bit %d is not valid with BitMask %d", _idx, m_BitMask);
	return "";
}

bool ValueBitSet::SetBitHelp
(
		uint8 _idx,
		string help
)
{
	if (isValidBit(_idx)) {
		m_BitHelpString[_idx] = help;
		return true;
	}
	Log::Write(LogLevel_Warning, m_id.GetNodeId(), "SetBitHelp: Bit %d is not valid with BitMask %d", _idx, m_BitMask);
	return false;
}

bool ValueBitSet::isValidBit
(
		uint8 _idx
) const
{
	if (((m_BitMask) & (1 << _idx)) == 0)
		return false;
	return true;
}

string ValueBitSet::GetBitLabel
(
		uint8 _idx
)
{
	if (isValidBit(_idx)) {
		return m_BitLabelString.at(_idx);
	}
	Log::Write(LogLevel_Warning, m_id.GetNodeId(), "GetBitLabel: Bit %d is not valid with BitMask %d", _idx, m_BitMask);
	return "";

}
bool ValueBitSet::SetBitLabel
(
		uint8 _idx,
		string label
)
{
	if (isValidBit(_idx)) {
		m_BitLabelString[_idx] = label;
		return true;
	}
	Log::Write(LogLevel_Warning, m_id.GetNodeId(), "SetBitLabel: Bit %d is not valid with BitMask %d", _idx, m_BitMask);
	return false;

}

uint8 ValueBitSet::GetSize
(
) const
{
	return m_size;
}
void ValueBitSet::SetSize
(
		uint8 size
)
{
	m_size = size;
}


//-----------------------------------------------------------------------------
// <ValueBitSet::OnValueRefreshed>
// A value in a device has been refreshed
//-----------------------------------------------------------------------------
void ValueBitSet::OnValueRefreshed
(
	uint32 const _value
)
{
	switch( VerifyRefreshedValue( (void*) &m_value, (void*) &m_valueCheck, (void*) &_value, ValueID::ValueType_Bool) )
	{
	case 0:		// value hasn't changed, nothing to do
		break;
	case 1:		// value has changed (not confirmed yet), save _value in m_valueCheck
		m_valueCheck = _value;
		break;
	case 2:		// value has changed (confirmed), save _value in m_value
		m_value = _value;
		break;
	case 3:		// all three values are different, so wait for next refresh to try again
		break;
	}
}