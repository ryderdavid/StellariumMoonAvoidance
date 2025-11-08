#include "MoonAvoidanceDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDoubleSpinBox>
#include <QColorDialog>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDebug>

MoonAvoidanceDialog::MoonAvoidanceDialog(QWidget* parent)
	: QDialog(parent)
	, tableWidget(nullptr)
	, addButton(nullptr)
	, removeButton(nullptr)
	, okButton(nullptr)
	, cancelButton(nullptr)
{
	qDebug() << "MoonAvoidanceDialog: Constructor called";
	
	// Initialize UI first
	setupUI();
	
	qDebug() << "MoonAvoidanceDialog: setupUI completed";
	
	// Verify widgets were created
	if (!tableWidget)
	{
		qFatal("MoonAvoidanceDialog: Failed to create tableWidget");
		return;
	}
	
	qDebug() << "MoonAvoidanceDialog: All widgets created successfully";
	
	setWindowTitle("Moon Avoidance Configuration");
	setMinimumSize(700, 400);
	
	qDebug() << "MoonAvoidanceDialog: Constructor finished";
}

MoonAvoidanceDialog::~MoonAvoidanceDialog()
{
}

void MoonAvoidanceDialog::setupUI()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(10);
	
	// Create table
	tableWidget = new QTableWidget(this);
	tableWidget->setColumnCount(7);
	QStringList headers;
	headers << "Filter Name" << "Separation" << "Width" << "Relaxation" << "Min Alt" << "Max Alt" << "Color";
	tableWidget->setHorizontalHeaderLabels(headers);
	tableWidget->horizontalHeader()->setStretchLastSection(true);
	tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	tableWidget->setMinimumSize(600, 300);
	
	mainLayout->addWidget(tableWidget);
	
	// Button layout
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	
	addButton = new QPushButton("Add Filter", this);
	removeButton = new QPushButton("Remove Filter", this);
	okButton = new QPushButton("OK", this);
	cancelButton = new QPushButton("Cancel", this);
	
	buttonLayout->addWidget(addButton);
	buttonLayout->addWidget(removeButton);
	buttonLayout->addStretch();
	buttonLayout->addWidget(okButton);
	buttonLayout->addWidget(cancelButton);
	
	mainLayout->addLayout(buttonLayout);
	
	// Connect signals
	connect(addButton, &QPushButton::clicked, this, &MoonAvoidanceDialog::addFilter);
	connect(removeButton, &QPushButton::clicked, this, &MoonAvoidanceDialog::removeFilter);
	connect(okButton, &QPushButton::clicked, this, [this]() {
		if (validateInput())
		{
			accept();
		}
	});
	connect(cancelButton, &QPushButton::clicked, this, &MoonAvoidanceDialog::reject);
	connect(tableWidget, &QTableWidget::cellChanged, this, &MoonAvoidanceDialog::updateFilterFromRow);
	connect(tableWidget, &QTableWidget::cellDoubleClicked, this, &MoonAvoidanceDialog::colorButtonClicked);
}

void MoonAvoidanceDialog::setFilters(const QList<FilterConfig>& filters)
{
	if (!tableWidget)
	{
		qWarning() << "MoonAvoidanceDialog: tableWidget is null in setFilters";
		return;
	}
	
	currentFilters = filters;
	populateTable(filters);
}

QList<FilterConfig> MoonAvoidanceDialog::getFilters() const
{
	return currentFilters;
}

void MoonAvoidanceDialog::populateTable(const QList<FilterConfig>& filters)
{
	if (!tableWidget)
	{
		qWarning() << "MoonAvoidanceDialog: tableWidget is null in populateTable";
		return;
	}
	
	// Disconnect signals temporarily to avoid triggering updates during population
	disconnect(tableWidget, &QTableWidget::cellChanged, this, &MoonAvoidanceDialog::updateFilterFromRow);
	
	// Clear existing rows
	tableWidget->setRowCount(0);
	tableWidget->setRowCount(filters.size());
	
	for (int i = 0; i < filters.size(); ++i)
	{
		const FilterConfig& filter = filters[i];
		
		// Filter name
		QTableWidgetItem* nameItem = new QTableWidgetItem(filter.name);
		tableWidget->setItem(i, 0, nameItem);
		
		// Separation
		QTableWidgetItem* sepItem = new QTableWidgetItem(QString::number(filter.separation));
		tableWidget->setItem(i, 1, sepItem);
		
		// Width
		QTableWidgetItem* widthItem = new QTableWidgetItem(QString::number(filter.width));
		tableWidget->setItem(i, 2, widthItem);
		
		// Relaxation
		QTableWidgetItem* relItem = new QTableWidgetItem(QString::number(filter.relaxation));
		tableWidget->setItem(i, 3, relItem);
		
		// Min Alt
		QTableWidgetItem* minAltItem = new QTableWidgetItem(QString::number(filter.minAlt));
		tableWidget->setItem(i, 4, minAltItem);
		
		// Max Alt
		QTableWidgetItem* maxAltItem = new QTableWidgetItem(QString::number(filter.maxAlt));
		tableWidget->setItem(i, 5, maxAltItem);
		
		// Color
		QTableWidgetItem* colorItem = new QTableWidgetItem();
		colorItem->setBackground(filter.color);
		colorItem->setText(filter.color.name());
		colorItem->setFlags(colorItem->flags() & ~Qt::ItemIsEditable);
		tableWidget->setItem(i, 6, colorItem);
	}
	
	// Reconnect signals after population is complete
	connect(tableWidget, &QTableWidget::cellChanged, this, &MoonAvoidanceDialog::updateFilterFromRow);
}

void MoonAvoidanceDialog::addFilter()
{
	if (!tableWidget)
	{
		qWarning() << "MoonAvoidanceDialog: tableWidget is null in addFilter";
		return;
	}
	
	int row = tableWidget->rowCount();
	tableWidget->insertRow(row);
	
	// Set default values
	FilterConfig newFilter("NewFilter", 35.0, 7.0, 1.0, -15.0, 5.0, Qt::white);
	
	QTableWidgetItem* nameItem = new QTableWidgetItem(newFilter.name);
	tableWidget->setItem(row, 0, nameItem);
	
	QTableWidgetItem* sepItem = new QTableWidgetItem(QString::number(newFilter.separation));
	tableWidget->setItem(row, 1, sepItem);
	
	QTableWidgetItem* widthItem = new QTableWidgetItem(QString::number(newFilter.width));
	tableWidget->setItem(row, 2, widthItem);
	
	QTableWidgetItem* relItem = new QTableWidgetItem(QString::number(newFilter.relaxation));
	tableWidget->setItem(row, 3, relItem);
	
	QTableWidgetItem* minAltItem = new QTableWidgetItem(QString::number(newFilter.minAlt));
	tableWidget->setItem(row, 4, minAltItem);
	
	QTableWidgetItem* maxAltItem = new QTableWidgetItem(QString::number(newFilter.maxAlt));
	tableWidget->setItem(row, 5, maxAltItem);
	
	QTableWidgetItem* colorItem = new QTableWidgetItem();
	colorItem->setBackground(newFilter.color);
	colorItem->setText(newFilter.color.name());
	colorItem->setFlags(colorItem->flags() & ~Qt::ItemIsEditable);
	tableWidget->setItem(row, 6, colorItem);
	
	currentFilters.append(newFilter);
}

void MoonAvoidanceDialog::removeFilter()
{
	int currentRow = tableWidget->currentRow();
	if (currentRow >= 0 && currentRow < tableWidget->rowCount() && currentRow < currentFilters.size())
	{
		tableWidget->removeRow(currentRow);
		currentFilters.removeAt(currentRow);
	}
}

void MoonAvoidanceDialog::colorButtonClicked()
{
	int row = tableWidget->currentRow();
	if (row < 0 || row >= tableWidget->rowCount())
		return;
	
	QColor currentColor = getColorFromRow(row);
	QColor newColor = QColorDialog::getColor(currentColor, this, "Select Filter Color");
	
	if (newColor.isValid())
	{
		setColorForRow(row, newColor);
		updateFilterFromRow(row);
	}
}

QColor MoonAvoidanceDialog::getColorFromRow(int row) const
{
	if (!tableWidget || row < 0 || row >= tableWidget->rowCount())
	{
		return Qt::white;
	}
	
	QTableWidgetItem* colorItem = tableWidget->item(row, 6);
	if (colorItem)
	{
		return colorItem->background().color();
	}
	return Qt::white;
}

void MoonAvoidanceDialog::setColorForRow(int row, const QColor& color)
{
	if (!tableWidget || row < 0 || row >= tableWidget->rowCount())
	{
		return;
	}
	
	QTableWidgetItem* colorItem = tableWidget->item(row, 6);
	if (colorItem)
	{
		colorItem->setBackground(color);
		colorItem->setText(color.name());
	}
}

void MoonAvoidanceDialog::updateFilterFromRow(int row)
{
	if (row < 0 || row >= tableWidget->rowCount())
		return;
	
	// Ensure currentFilters has enough entries
	while (currentFilters.size() <= row)
	{
		currentFilters.append(FilterConfig());
	}
	
	QTableWidgetItem* nameItem = tableWidget->item(row, 0);
	QTableWidgetItem* sepItem = tableWidget->item(row, 1);
	QTableWidgetItem* widthItem = tableWidget->item(row, 2);
	QTableWidgetItem* relItem = tableWidget->item(row, 3);
	QTableWidgetItem* minAltItem = tableWidget->item(row, 4);
	QTableWidgetItem* maxAltItem = tableWidget->item(row, 5);
	
	if (nameItem && sepItem && widthItem && relItem && minAltItem && maxAltItem)
	{
		FilterConfig& filter = currentFilters[row];
		filter.name = nameItem->text();
		filter.separation = sepItem->text().toDouble();
		filter.width = widthItem->text().toDouble();
		filter.relaxation = relItem->text().toDouble();
		filter.minAlt = minAltItem->text().toDouble();
		filter.maxAlt = maxAltItem->text().toDouble();
		filter.color = getColorFromRow(row);
	}
}

bool MoonAvoidanceDialog::validateInput()
{
	if (!tableWidget)
	{
		qWarning() << "MoonAvoidanceDialog: tableWidget is null in validateInput";
		return false;
	}
	
	// Basic validation - can be expanded
	for (int i = 0; i < tableWidget->rowCount(); ++i)
	{
		QTableWidgetItem* nameItem = tableWidget->item(i, 0);
		if (!nameItem || nameItem->text().isEmpty())
		{
			QMessageBox::warning(this, "Validation Error", 
				QString("Filter name cannot be empty (row %1)").arg(i + 1));
			return false;
		}
	}
	return true;
}

