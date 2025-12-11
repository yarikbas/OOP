"""
Report Generation Module
Generates PDF reports for voyages, cargo, and financial data
"""

from reportlab.lib.pagesizes import A4, letter
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.lib import colors
from reportlab.platypus import SimpleDocTemplate, Table, TableStyle, Paragraph, Spacer, PageBreak
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_RIGHT
from datetime import datetime
from typing import List, Dict
from io import BytesIO


class ReportGenerator:
    """Generate PDF reports for maritime operations"""
    
    @staticmethod
    def create_voyage_report(voyage_data: Dict, output: BytesIO = None) -> BytesIO:
        """
        Generate a voyage report PDF
        
        Args:
            voyage_data: Dictionary with voyage information
            output: BytesIO object to write to (creates new if None)
            
        Returns:
            BytesIO with PDF content
        """
        if output is None:
            output = BytesIO()
        
        doc = SimpleDocTemplate(output, pagesize=A4,
                               topMargin=0.5*inch, bottomMargin=0.5*inch)
        
        story = []
        styles = getSampleStyleSheet()
        
        # Title
        title_style = ParagraphStyle(
            'CustomTitle',
            parent=styles['Heading1'],
            fontSize=24,
            textColor=colors.HexColor('#1f4788'),
            spaceAfter=30,
            alignment=TA_CENTER
        )
        
        story.append(Paragraph("Voyage Report", title_style))
        story.append(Spacer(1, 0.3*inch))
        
        # Voyage details
        details = [
            ['Ship:', voyage_data.get('ship_name', 'N/A')],
            ['Route:', f"{voyage_data.get('origin_name', 'N/A')} → {voyage_data.get('destination_name', 'N/A')}"],
            ['Departure:', voyage_data.get('departed_at', 'N/A')],
            ['Arrival:', voyage_data.get('arrived_at', 'N/A')],
            ['Duration:', f"{voyage_data.get('duration_hours', 0)} hours"],
            ['Distance:', f"{voyage_data.get('distance_km', 0)} km"],
            ['Fuel Consumed:', f"{voyage_data.get('fuel_consumed_liters', 0)} liters"],
        ]
        
        details_table = Table(details, colWidths=[2*inch, 4*inch])
        details_table.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (0, -1), colors.HexColor('#e6f2ff')),
            ('TEXTCOLOR', (0, 0), (-1, -1), colors.black),
            ('ALIGN', (0, 0), (0, -1), 'RIGHT'),
            ('ALIGN', (1, 0), (1, -1), 'LEFT'),
            ('FONTNAME', (0, 0), (-1, -1), 'Helvetica'),
            ('FONTSIZE', (0, 0), (-1, -1), 12),
            ('BOTTOMPADDING', (0, 0), (-1, -1), 12),
            ('GRID', (0, 0), (-1, -1), 1, colors.grey),
        ]))
        
        story.append(details_table)
        story.append(Spacer(1, 0.3*inch))
        
        # Financial summary
        story.append(Paragraph("Financial Summary", styles['Heading2']))
        story.append(Spacer(1, 0.1*inch))
        
        financial = [
            ['Category', 'Amount (USD)'],
            ['Total Cost', f"${voyage_data.get('total_cost_usd', 0):,.2f}"],
            ['Total Revenue', f"${voyage_data.get('total_revenue_usd', 0):,.2f}"],
            ['Profit/Loss', f"${voyage_data.get('total_revenue_usd', 0) - voyage_data.get('total_cost_usd', 0):,.2f}"],
        ]
        
        financial_table = Table(financial, colWidths=[3*inch, 3*inch])
        financial_table.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1f4788')),
            ('TEXTCOLOR', (0, 0), (-1, 0), colors.whitesmoke),
            ('ALIGN', (0, 0), (-1, -1), 'CENTER'),
            ('FONTNAME', (0, 0), (-1, 0), 'Helvetica-Bold'),
            ('FONTSIZE', (0, 0), (-1, 0), 14),
            ('BOTTOMPADDING', (0, 0), (-1, 0), 12),
            ('GRID', (0, 0), (-1, -1), 1, colors.black),
        ]))
        
        story.append(financial_table)
        story.append(Spacer(1, 0.3*inch))
        
        # Notes
        if voyage_data.get('notes'):
            story.append(Paragraph("Notes", styles['Heading2']))
            story.append(Paragraph(voyage_data['notes'], styles['BodyText']))
        
        # Footer
        story.append(Spacer(1, 0.5*inch))
        footer_style = ParagraphStyle(
            'Footer',
            parent=styles['Normal'],
            fontSize=10,
            textColor=colors.grey,
            alignment=TA_CENTER
        )
        story.append(Paragraph(f"Generated on {datetime.now().strftime('%Y-%m-%d %H:%M')}", footer_style))
        
        doc.build(story)
        output.seek(0)
        return output
    
    @staticmethod
    def create_cargo_manifest(cargo_list: List[Dict], output: BytesIO = None) -> BytesIO:
        """
        Generate a cargo manifest PDF
        
        Args:
            cargo_list: List of cargo dictionaries
            output: BytesIO object to write to
            
        Returns:
            BytesIO with PDF content
        """
        if output is None:
            output = BytesIO()
        
        doc = SimpleDocTemplate(output, pagesize=A4,
                               topMargin=0.5*inch, bottomMargin=0.5*inch)
        
        story = []
        styles = getSampleStyleSheet()
        
        # Title
        title_style = ParagraphStyle(
            'CustomTitle',
            parent=styles['Heading1'],
            fontSize=24,
            textColor=colors.HexColor('#1f4788'),
            spaceAfter=30,
            alignment=TA_CENTER
        )
        
        story.append(Paragraph("Cargo Manifest", title_style))
        story.append(Spacer(1, 0.3*inch))
        
        # Summary
        total_weight = sum(c.get('weight_tonnes', 0) for c in cargo_list)
        total_volume = sum(c.get('volume_m3', 0) for c in cargo_list)
        total_value = sum(c.get('value_usd', 0) for c in cargo_list)
        
        summary = [
            ['Total Items:', str(len(cargo_list))],
            ['Total Weight:', f"{total_weight:.2f} tonnes"],
            ['Total Volume:', f"{total_volume:.2f} m³"],
            ['Total Value:', f"${total_value:,.2f}"],
        ]
        
        summary_table = Table(summary, colWidths=[2*inch, 3*inch])
        summary_table.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (0, -1), colors.HexColor('#e6f2ff')),
            ('ALIGN', (0, 0), (0, -1), 'RIGHT'),
            ('ALIGN', (1, 0), (1, -1), 'LEFT'),
            ('FONTNAME', (0, 0), (-1, -1), 'Helvetica-Bold'),
            ('FONTSIZE', (0, 0), (-1, -1), 12),
            ('GRID', (0, 0), (-1, -1), 1, colors.grey),
        ]))
        
        story.append(summary_table)
        story.append(Spacer(1, 0.3*inch))
        
        # Cargo list
        story.append(Paragraph("Cargo Items", styles['Heading2']))
        story.append(Spacer(1, 0.1*inch))
        
        cargo_data = [['Name', 'Type', 'Weight (t)', 'Volume (m³)', 'Status']]
        
        for cargo in cargo_list:
            cargo_data.append([
                cargo.get('name', 'N/A'),
                cargo.get('type', 'N/A'),
                f"{cargo.get('weight_tonnes', 0):.1f}",
                f"{cargo.get('volume_m3', 0):.1f}",
                cargo.get('status', 'N/A')
            ])
        
        cargo_table = Table(cargo_data, colWidths=[1.8*inch, 1.5*inch, 1*inch, 1*inch, 1*inch])
        cargo_table.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1f4788')),
            ('TEXTCOLOR', (0, 0), (-1, 0), colors.whitesmoke),
            ('ALIGN', (0, 0), (-1, -1), 'CENTER'),
            ('FONTNAME', (0, 0), (-1, 0), 'Helvetica-Bold'),
            ('FONTSIZE', (0, 0), (-1, 0), 11),
            ('FONTSIZE', (0, 1), (-1, -1), 9),
            ('BOTTOMPADDING', (0, 0), (-1, 0), 12),
            ('GRID', (0, 0), (-1, -1), 1, colors.black),
            ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.white, colors.HexColor('#f0f0f0')]),
        ]))
        
        story.append(cargo_table)
        
        # Footer
        story.append(Spacer(1, 0.5*inch))
        footer_style = ParagraphStyle(
            'Footer',
            parent=styles['Normal'],
            fontSize=10,
            textColor=colors.grey,
            alignment=TA_CENTER
        )
        story.append(Paragraph(f"Generated on {datetime.now().strftime('%Y-%m-%d %H:%M')}", footer_style))
        
        doc.build(story)
        output.seek(0)
        return output


if __name__ == "__main__":
    # Demo
    print("Report Generator Demo")
    print("=" * 50)
    
    # Test voyage report
    voyage = {
        'ship_name': 'SS Atlantic',
        'origin_name': 'Odesa',
        'destination_name': 'Istanbul',
        'departed_at': '2024-01-15 10:00',
        'arrived_at': '2024-01-16 08:00',
        'duration_hours': 22,
        'distance_km': 450,
        'fuel_consumed_liters': 5000,
        'total_cost_usd': 15000,
        'total_revenue_usd': 25000,
        'notes': 'Successful voyage with favorable weather conditions.'
    }
    
    output = ReportGenerator.create_voyage_report(voyage)
    
    with open('test_voyage_report.pdf', 'wb') as f:
        f.write(output.read())
    
    print("\n✅ Test voyage report generated: test_voyage_report.pdf")
    
    # Test cargo manifest
    cargo_list = [
        {'name': 'Steel Coils', 'type': 'Metal', 'weight_tonnes': 50, 'volume_m3': 30, 'value_usd': 50000, 'status': 'loaded'},
        {'name': 'Electronics', 'type': 'Technology', 'weight_tonnes': 5, 'volume_m3': 15, 'value_usd': 100000, 'status': 'loaded'},
    ]
    
    output = ReportGenerator.create_cargo_manifest(cargo_list)
    
    with open('test_cargo_manifest.pdf', 'wb') as f:
        f.write(output.read())
    
    print("✅ Test cargo manifest generated: test_cargo_manifest.pdf")
