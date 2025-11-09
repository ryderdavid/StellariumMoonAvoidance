#include "MoonAvoidanceDialog.hpp"
#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "MoonAvoidance.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QColorDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QDebug>
#include <cmath>

MoonAvoidanceDialog::MoonAvoidanceDialog()
	: StelDialog("MoonAvoidance")
	, filterListWidget(nullptr)
	, formLayout(nullptr)
	, separationSpinBox(nullptr)
	, widthSpinBox(nullptr)
	, relaxationSpinBox(nullptr)
	, minAltSpinBox(nullptr)
	, maxAltSpinBox(nullptr)
	, colorButton(nullptr)
	, colorLabel(nullptr)
	, moonAgeLabel(nullptr)
	, currentSeparationLabel(nullptr)
	, addButton(nullptr)
	, removeButton(nullptr)
	, okButton(nullptr)
	, cancelButton(nullptr)
	, currentFilterIndex(-1)
	, accepted(false)
{
	qDebug() << "MoonAvoidanceDialog: Constructor called";
}

MoonAvoidanceDialog::~MoonAvoidanceDialog()
{
}

void MoonAvoidanceDialog::createDialogContent()
{
	qDebug() << "MoonAvoidanceDialog: createDialogContent called";
	
	// Create the dialog widget
	dialog = new QWidget();
	dialog->setMinimumSize(700, 500);
	
	QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	
	// Add TitleBar
	TitleBar* titleBar = new TitleBar(dialog);
	titleBar->setTitle("Moon Avoidance");
	mainLayout->addWidget(titleBar);
	
	// Connect title bar signals
	connect(titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(titleBar, &TitleBar::movedTo, this, &StelDialog::handleMovedTo);
	
	// Content layout
	QVBoxLayout* contentLayout = new QVBoxLayout();
	contentLayout->setContentsMargins(10, 10, 10, 10);
	contentLayout->setSpacing(10);
	
	// Create horizontal layout for list and form
	QHBoxLayout* listFormLayout = new QHBoxLayout();
	
	// Left panel: Filter list
	filterListWidget = new QListWidget(dialog);
	if (!filterListWidget)
	{
		qWarning() << "MoonAvoidanceDialog: Failed to create QListWidget";
		return;
	}
	filterListWidget->setMinimumWidth(150);
	filterListWidget->setMaximumWidth(200);
	listFormLayout->addWidget(filterListWidget);
	
	// Right panel: Form fields
	QVBoxLayout* formContainerLayout = new QVBoxLayout();
	formLayout = new QFormLayout();
	
	// Separation (in degrees)
	separationSpinBox = new QDoubleSpinBox(dialog);
	separationSpinBox->setRange(0.0, 180.0);
	separationSpinBox->setDecimals(1);
	separationSpinBox->setSuffix("°");
	formLayout->addRow("Separation:", separationSpinBox);
	
	// Width (in days)
	widthSpinBox = new QDoubleSpinBox(dialog);
	widthSpinBox->setRange(0.1, 30.0);
	widthSpinBox->setDecimals(1);
	widthSpinBox->setSuffix(" days");
	formLayout->addRow("Width:", widthSpinBox);
	
	// Relaxation
	relaxationSpinBox = new QDoubleSpinBox(dialog);
	relaxationSpinBox->setRange(0.0, 100.0);
	relaxationSpinBox->setDecimals(1);
	formLayout->addRow("Relaxation:", relaxationSpinBox);
	
	// Min Alt
	minAltSpinBox = new QDoubleSpinBox(dialog);
	minAltSpinBox->setRange(-90.0, 90.0);
	minAltSpinBox->setDecimals(1);
	minAltSpinBox->setSuffix("°");
	formLayout->addRow("Min Altitude:", minAltSpinBox);
	
	// Max Alt
	maxAltSpinBox = new QDoubleSpinBox(dialog);
	maxAltSpinBox->setRange(-90.0, 90.0);
	maxAltSpinBox->setDecimals(1);
	maxAltSpinBox->setSuffix("°");
	formLayout->addRow("Max Altitude:", maxAltSpinBox);
	
	// Moon Age (calculated, read-only) - days since new moon
	moonAgeLabel = new QLabel(dialog);
	moonAgeLabel->setText("--");
	moonAgeLabel->setStyleSheet("QLabel { background-color: #2a2a2a; padding: 4px; border: 1px solid #555555; border-radius: 3px; }");
	moonAgeLabel->setMinimumHeight(25);
	formLayout->addRow("Moon Age:", moonAgeLabel);
	
	// Current Separation (calculated, read-only)
	currentSeparationLabel = new QLabel(dialog);
	currentSeparationLabel->setText("--");
	currentSeparationLabel->setStyleSheet("QLabel { background-color: #2a2a2a; padding: 4px; border: 1px solid #555555; border-radius: 3px; }");
	currentSeparationLabel->setMinimumHeight(25);
	formLayout->addRow("Current Separation:", currentSeparationLabel);
	
	// Color
	colorButton = new QPushButton("Choose Color", dialog);
	colorLabel = new QLabel(dialog);
	colorLabel->setMinimumSize(50, 30);
	colorLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	QHBoxLayout* colorLayout = new QHBoxLayout();
	colorLayout->addWidget(colorButton);
	colorLayout->addWidget(colorLabel);
	colorLayout->addStretch();
	formLayout->addRow("Color:", colorLayout);
	
	formContainerLayout->addLayout(formLayout);
	formContainerLayout->addStretch();
	
	listFormLayout->addLayout(formContainerLayout, 1);
	contentLayout->addLayout(listFormLayout);
	
	// Button layout
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	
	addButton = new QPushButton("Add Filter", dialog);
	removeButton = new QPushButton("Remove Filter", dialog);
	okButton = new QPushButton("OK", dialog);
	cancelButton = new QPushButton("Cancel", dialog);
	
	if (!addButton || !removeButton || !okButton || !cancelButton)
	{
		qWarning() << "MoonAvoidanceDialog: Failed to create buttons";
		return;
	}
	
	// Ensure buttons are readable - set proper text colors and styles
	QString buttonStyle = "QPushButton { "
		"background-color: #3a3a3a; "
		"color: white; "
		"border: 1px solid #555555; "
		"border-radius: 4px; "
		"padding: 6px 12px; "
		"min-width: 80px; "
		"} "
		"QPushButton:hover { "
		"background-color: #4a4a4a; "
		"} "
		"QPushButton:pressed { "
		"background-color: #2a2a2a; "
		"}";
	
	addButton->setStyleSheet(buttonStyle);
	removeButton->setStyleSheet(buttonStyle);
	okButton->setStyleSheet(buttonStyle);
	cancelButton->setStyleSheet(buttonStyle);
	colorButton->setStyleSheet(buttonStyle);
	
	buttonLayout->addWidget(addButton);
	buttonLayout->addWidget(removeButton);
	buttonLayout->addStretch();
	buttonLayout->addWidget(okButton);
	buttonLayout->addWidget(cancelButton);
	
	contentLayout->addLayout(buttonLayout);
	mainLayout->addLayout(contentLayout);
	
	// Connect signals
	connect(addButton, &QPushButton::clicked, this, &MoonAvoidanceDialog::addFilter);
	connect(removeButton, &QPushButton::clicked, this, &MoonAvoidanceDialog::removeFilter);
	connect(okButton, &QPushButton::clicked, this, [this]() {
		if (validateInput())
		{
			accepted = true;
			close();
		}
	});
	connect(cancelButton, &QPushButton::clicked, this, [this]() {
		accepted = false;
		close();
	});
	
	if (filterListWidget)
	{
		connect(filterListWidget, &QListWidget::currentTextChanged, this, &MoonAvoidanceDialog::onFilterSelectionChanged);
	}
	
	if (separationSpinBox)
	{
		connect(separationSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MoonAvoidanceDialog::updateSeparation);
	}
	if (widthSpinBox)
	{
		connect(widthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MoonAvoidanceDialog::updateWidth);
	}
	if (relaxationSpinBox)
	{
		connect(relaxationSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MoonAvoidanceDialog::updateRelaxation);
	}
	if (minAltSpinBox)
	{
		connect(minAltSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MoonAvoidanceDialog::updateMinAlt);
	}
	if (maxAltSpinBox)
	{
		connect(maxAltSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MoonAvoidanceDialog::updateMaxAlt);
	}
	if (colorButton)
	{
		connect(colorButton, &QPushButton::clicked, this, &MoonAvoidanceDialog::updateColor);
	}
	
	// Connect language change
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	
	// Initially disable form fields until a filter is selected
	enableFormFields(false);
	
	// If we have pending filters (set before dialog was created), apply them now
	if (!pendingFilters.isEmpty())
	{
		setFilters(pendingFilters);
		pendingFilters.clear();
	}
	
	qDebug() << "MoonAvoidanceDialog: createDialogContent completed";
}

void MoonAvoidanceDialog::retranslate()
{
	// Retranslate dialog content if needed
	// For now, we'll keep the English text
}

void MoonAvoidanceDialog::setFilters(const QList<FilterConfig>& filters)
{
	// If dialog content hasn't been created yet, store filters for later
	if (!filterListWidget || !dialog)
	{
		pendingFilters = filters;
		qDebug() << "MoonAvoidanceDialog: Dialog not ready, storing" << filters.size() << "filters for later";
		return;
	}
	
	currentFilters = filters;
	
	// Populate list widget
	filterListWidget->blockSignals(true);
	filterListWidget->clear();
	
	for (const FilterConfig& filter : filters)
	{
		filterListWidget->addItem(filter.name);
	}
	
	filterListWidget->blockSignals(false);
	
	// Select first filter if available
	if (filters.size() > 0)
	{
		filterListWidget->setCurrentRow(0);
		onFilterSelectionChanged(filters[0].name);
	}
	else
	{
		enableFormFields(false);
	}
}

QList<FilterConfig> MoonAvoidanceDialog::getFilters() const
{
	return currentFilters;
}

void MoonAvoidanceDialog::onFilterSelectionChanged(const QString& filterName)
{
	if (filterName.isEmpty())
	{
		enableFormFields(false);
		currentFilterName.clear();
		currentFilterIndex = -1;
		return;
	}
	
	// Find the filter index
	currentFilterIndex = -1;
	for (int i = 0; i < currentFilters.size(); ++i)
	{
		if (currentFilters[i].name == filterName)
		{
			currentFilterIndex = i;
			currentFilterName = filterName;
			break;
		}
	}
	
	if (currentFilterIndex >= 0)
	{
		updateFormFields();
		enableFormFields(true);
	}
	else
	{
		enableFormFields(false);
	}
}

void MoonAvoidanceDialog::updateFormFields()
{
	if (currentFilterIndex < 0 || currentFilterIndex >= currentFilters.size())
	{
		return;
	}
	
	const FilterConfig& filter = currentFilters[currentFilterIndex];
	
	// Block signals to avoid triggering updates while setting values
	if (separationSpinBox)
	{
		separationSpinBox->blockSignals(true);
		separationSpinBox->setValue(filter.separation);
		separationSpinBox->blockSignals(false);
	}
	
	if (widthSpinBox)
	{
		widthSpinBox->blockSignals(true);
		widthSpinBox->setValue(filter.width);
		widthSpinBox->blockSignals(false);
	}
	
	if (relaxationSpinBox)
	{
		relaxationSpinBox->blockSignals(true);
		relaxationSpinBox->setValue(filter.relaxation);
		relaxationSpinBox->blockSignals(false);
	}
	
	if (minAltSpinBox)
	{
		minAltSpinBox->blockSignals(true);
		minAltSpinBox->setValue(filter.minAlt);
		minAltSpinBox->blockSignals(false);
	}
	
	if (maxAltSpinBox)
	{
		maxAltSpinBox->blockSignals(true);
		maxAltSpinBox->setValue(filter.maxAlt);
		maxAltSpinBox->blockSignals(false);
	}
	
	if (colorLabel)
	{
		QString style = QString("background-color: %1;").arg(filter.color.name());
		colorLabel->setStyleSheet(style);
		colorLabel->setText(filter.color.name());
	}
	
	// Update calculated current separation and moon age
	updateCurrentSeparation();
}

void MoonAvoidanceDialog::enableFormFields(bool enabled)
{
	if (separationSpinBox) separationSpinBox->setEnabled(enabled);
	if (widthSpinBox) widthSpinBox->setEnabled(enabled);
	if (relaxationSpinBox) relaxationSpinBox->setEnabled(enabled);
	if (minAltSpinBox) minAltSpinBox->setEnabled(enabled);
	if (maxAltSpinBox) maxAltSpinBox->setEnabled(enabled);
	if (colorButton) colorButton->setEnabled(enabled);
	if (removeButton) removeButton->setEnabled(enabled && currentFilterIndex >= 0);
}

void MoonAvoidanceDialog::addFilter()
{
	// Create a new filter with default values
	FilterConfig newFilter;
	newFilter.name = QString("Filter %1").arg(currentFilters.size() + 1);
	newFilter.separation = 140.0;
	newFilter.width = 14.0;
	newFilter.relaxation = 2.0;
	newFilter.minAlt = -15.0;
	newFilter.maxAlt = 5.0;
	newFilter.color = QColor(Qt::white);
	
	currentFilters.append(newFilter);
	
	// Add to list widget
	if (filterListWidget)
	{
		filterListWidget->addItem(newFilter.name);
		filterListWidget->setCurrentRow(currentFilters.size() - 1);
		onFilterSelectionChanged(newFilter.name);
	}
}

void MoonAvoidanceDialog::removeFilter()
{
	if (currentFilterIndex < 0 || currentFilterIndex >= currentFilters.size())
	{
		return;
	}
	
	// Remove from list
	currentFilters.removeAt(currentFilterIndex);
	
	// Remove from list widget
	if (filterListWidget)
	{
		QListWidgetItem* item = filterListWidget->takeItem(currentFilterIndex);
		delete item;
		
		// Select next item or previous if at end
		if (currentFilters.size() > 0)
		{
			int newIndex = (currentFilterIndex < currentFilters.size()) ? currentFilterIndex : currentFilters.size() - 1;
			filterListWidget->setCurrentRow(newIndex);
			if (newIndex >= 0)
			{
				onFilterSelectionChanged(currentFilters[newIndex].name);
			}
		}
		else
		{
			enableFormFields(false);
			currentFilterName.clear();
			currentFilterIndex = -1;
		}
	}
}

void MoonAvoidanceDialog::updateSeparation(double value)
{
	if (currentFilterIndex >= 0 && currentFilterIndex < currentFilters.size())
	{
		currentFilters[currentFilterIndex].separation = value;
		updateCurrentSeparation();
	}
}

void MoonAvoidanceDialog::updateWidth(double value)
{
	if (currentFilterIndex >= 0 && currentFilterIndex < currentFilters.size())
	{
		currentFilters[currentFilterIndex].width = value;
		updateCurrentSeparation();
	}
}

void MoonAvoidanceDialog::updateRelaxation(double value)
{
	if (currentFilterIndex >= 0 && currentFilterIndex < currentFilters.size())
	{
		currentFilters[currentFilterIndex].relaxation = value;
		updateCurrentSeparation();
	}
}

void MoonAvoidanceDialog::updateMinAlt(double value)
{
	if (currentFilterIndex >= 0 && currentFilterIndex < currentFilters.size())
	{
		currentFilters[currentFilterIndex].minAlt = value;
		updateCurrentSeparation();
	}
}

void MoonAvoidanceDialog::updateMaxAlt(double value)
{
	if (currentFilterIndex >= 0 && currentFilterIndex < currentFilters.size())
	{
		currentFilters[currentFilterIndex].maxAlt = value;
		updateCurrentSeparation();
	}
}

void MoonAvoidanceDialog::updateCurrentSeparation()
{
	if (!currentSeparationLabel || currentFilterIndex < 0 || currentFilterIndex >= currentFilters.size())
	{
		if (currentSeparationLabel)
			currentSeparationLabel->setText("--");
		return;
	}
	
	// Get the MoonAvoidance plugin instance to access current moon data
	MoonAvoidance* plugin = qobject_cast<MoonAvoidance*>(StelApp::getInstance().getModuleMgr().getModule("MoonAvoidance"));
	if (!plugin)
	{
		currentSeparationLabel->setText("N/A");
		return;
	}
	
	// Get current moon age and altitude from the plugin
	double moonAgeSinceNewMoon = plugin->getCurrentMoonAgeDays(); // Days since new moon
	double moonAgeFromFullMoon = plugin->getCurrentMoonAgeFromFullDays(); // Days from full moon (for formula)
	double moonAltitude = plugin->getCurrentMoonAltitude();
	
	// Update moon age display
	if (moonAgeLabel)
	{
		moonAgeLabel->setText(QString("%1 days").arg(moonAgeSinceNewMoon, 0, 'f', 2));
	}
	
	const FilterConfig& filter = currentFilters[currentFilterIndex];
	
	// Calculate adjusted separation based on altitude (same as in MoonAvoidance::calculateSeparation)
	double adjustedSeparation = filter.separation + filter.relaxation * (moonAltitude - filter.maxAlt);
	
	// Calculate adjusted width based on altitude (same as in MoonAvoidance::calculateWidth)
	double denominator = filter.maxAlt - filter.minAlt;
	double adjustedWidth = filter.width;
	if (denominator != 0.0)
	{
		double ratio = (moonAltitude - filter.minAlt) / denominator;
		adjustedWidth = filter.width * ratio;
	}
	
	// Apply the Moon Avoidance Lorentzian formula from the spreadsheet:
	// SEPARATION = DISTANCE / ( 1 + POWER( ( ( 0.5 - ( AGE / 29.5) ) / ( WIDTH / 29.5 ) ) , 2 ) )
	const double SYNODIC_PERIOD = 29.5;
	
	// Avoid division by zero
	if (adjustedWidth <= 0.0)
		adjustedWidth = 1.0;
	
	// Use the corrected formula: SEPARATION = DISTANCE / ( 1 + (AGE / WIDTH)^2 )
	// where AGE is days from full moon (0 = full moon, 14.765 = new moon)
	// This gives highest separation at full moon and lower as we move away
	double normalizedTerm = moonAgeFromFullMoon / adjustedWidth;
	
	// Calculate the Lorentzian factor: 1 + (normalizedTerm)^2
	double lorentzianFactor = 1.0 + (normalizedTerm * normalizedTerm);
	
	// Calculate the current separation in degrees
	double currentSeparationDegrees = adjustedSeparation / lorentzianFactor;
	
	// Ensure minimum separation (at least 1 degree)
	if (currentSeparationDegrees < 1.0)
		currentSeparationDegrees = 1.0;
	
	// Display the calculated separation
	currentSeparationLabel->setText(QString("%1°").arg(currentSeparationDegrees, 0, 'f', 1));
}

void MoonAvoidanceDialog::updateColor()
{
	if (currentFilterIndex < 0 || currentFilterIndex >= currentFilters.size())
	{
		return;
	}
	
	QColor currentColor = currentFilters[currentFilterIndex].color;
	QColor newColor = QColorDialog::getColor(currentColor, dialog, "Choose Filter Color");
	
	if (newColor.isValid())
	{
		currentFilters[currentFilterIndex].color = newColor;
		if (colorLabel)
		{
			QString style = QString("background-color: %1;").arg(newColor.name());
			colorLabel->setStyleSheet(style);
			colorLabel->setText(newColor.name());
		}
	}
}

bool MoonAvoidanceDialog::validateInput()
{
	// Validate all filters
	for (const FilterConfig& filter : currentFilters)
	{
		if (filter.name.isEmpty())
		{
			QMessageBox::warning(dialog, "Validation Error", "Filter name cannot be empty.");
			return false;
		}
		
		if (filter.separation < 0)
		{
			QMessageBox::warning(dialog, "Validation Error", QString("Separation must be non-negative for filter '%1'.").arg(filter.name));
			return false;
		}
		
		if (filter.width < 0)
		{
			QMessageBox::warning(dialog, "Validation Error", QString("Width must be non-negative for filter '%1'.").arg(filter.name));
			return false;
		}
		
		if (filter.minAlt >= filter.maxAlt)
		{
			QMessageBox::warning(dialog, "Validation Error", QString("Min Altitude must be less than Max Altitude for filter '%1'.").arg(filter.name));
			return false;
		}
	}
	
	return true;
}
