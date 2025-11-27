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
#include <QTabWidget>
#include <QTextBrowser>
#include <QGroupBox>
#include <QDebug>
#include <cmath>

MoonAvoidanceDialog::MoonAvoidanceDialog()
	: StelDialog("MoonAvoidance")
	, tabWidget(nullptr)
	, filtersTab(nullptr)
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
	, enabledCheckBox(nullptr)
	, infoTab(nullptr)
	, aboutTab(nullptr)
	, diagramTab(nullptr)
	, infoTextBrowser(nullptr)
	, aboutTextBrowser(nullptr)
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
	// Disconnect from plugin to prevent dangling pointers
	MoonAvoidance* plugin = qobject_cast<MoonAvoidance*>(StelApp::getInstance().getModuleMgr().getModule("MoonAvoidance"));
	if (plugin)
	{
		// Disconnect everything where the plugin is the sender and this dialog (or its children) is the receiver
		plugin->disconnect(this);
		if (enabledCheckBox)
		{
			plugin->disconnect(enabledCheckBox);
		}
	}
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

	// Create tab widget (no content layout wrapper)
	tabWidget = new QTabWidget(dialog);
	tabWidget->setDocumentMode(false);

	// Create tabs
	createFiltersTab();
	createInfoTab();
	createAboutTab();
	createDiagramTab();

	// Add tabs to tab widget
	if (filtersTab)
		tabWidget->addTab(filtersTab, "Filters");
	if (infoTab)
		tabWidget->addTab(infoTab, "Info");
	if (aboutTab)
		tabWidget->addTab(aboutTab, "About");
	if (diagramTab)
		tabWidget->addTab(diagramTab, "Diagram");

	mainLayout->addWidget(tabWidget);

	// Add OK/Cancel buttons at the bottom (inside main layout)
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	okButton = new QPushButton("OK", dialog);
	cancelButton = new QPushButton("Cancel", dialog);

	buttonLayout->addStretch();
	buttonLayout->addWidget(okButton);
	buttonLayout->addWidget(cancelButton);

	// Create a widget to hold the button layout with some padding
	QWidget* buttonWidget = new QWidget(dialog);
	QVBoxLayout* buttonWidgetLayout = new QVBoxLayout(buttonWidget);
	buttonWidgetLayout->setContentsMargins(10, 5, 10, 5);
	buttonWidgetLayout->addLayout(buttonLayout);

	mainLayout->addWidget(buttonWidget);

	// Connect OK/Cancel signals
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

	// Connect language change
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	qDebug() << "MoonAvoidanceDialog: createDialogContent completed";
}

void MoonAvoidanceDialog::retranslate()
{
	// Retranslate dialog content if needed
	// For now, we'll keep the English text
}

void MoonAvoidanceDialog::createFiltersTab()
{
	filtersTab = new QWidget();
	QVBoxLayout* filtersLayout = new QVBoxLayout(filtersTab);

	// Create a flat QGroupBox to hold the filter configuration
	QGroupBox* filterGroupBox = new QGroupBox("Filter Configuration", filtersTab);
	filterGroupBox->setFlat(true);
	QVBoxLayout* groupLayout = new QVBoxLayout(filterGroupBox);

	// Add visibility checkbox at the top
	enabledCheckBox = new QCheckBox("Show Moon Avoidance Circles", filterGroupBox);
	if (enabledCheckBox)
	{
		groupLayout->addWidget(enabledCheckBox);

		// Connect visibility checkbox to MoonAvoidance plugin
		MoonAvoidance* plugin = qobject_cast<MoonAvoidance*>(StelApp::getInstance().getModuleMgr().getModule("MoonAvoidance"));
		if (plugin)
		{
			// Set initial state from plugin
			enabledCheckBox->setChecked(plugin->isEnabled());

			// Connect checkbox to plugin
			connect(enabledCheckBox, &QCheckBox::toggled, plugin, &MoonAvoidance::setEnabled);

			// Connect plugin to checkbox (for updates from other sources)
			connect(plugin, &MoonAvoidance::enabledChanged, enabledCheckBox, &QCheckBox::setChecked);

			qDebug() << "MoonAvoidanceDialog: Visibility checkbox connected to plugin";
		}
		else
		{
			qWarning() << "MoonAvoidanceDialog: Failed to connect visibility checkbox - plugin is null";
		}
	}

	// Create horizontal layout for list and form
	QHBoxLayout* listFormLayout = new QHBoxLayout();

	// Left panel: Filter list
	filterListWidget = new QListWidget(filterGroupBox);
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
	separationSpinBox = new QDoubleSpinBox(filterGroupBox);
	separationSpinBox->setRange(0.0, 180.0);
	separationSpinBox->setDecimals(1);
	separationSpinBox->setSuffix("°");
	formLayout->addRow("Separation:", separationSpinBox);

	// Width (in days)
	widthSpinBox = new QDoubleSpinBox(filterGroupBox);
	widthSpinBox->setRange(0.1, 30.0);
	widthSpinBox->setDecimals(1);
	widthSpinBox->setSuffix(" days");
	formLayout->addRow("Width:", widthSpinBox);

	// Relaxation
	relaxationSpinBox = new QDoubleSpinBox(filterGroupBox);
	relaxationSpinBox->setRange(0.0, 100.0);
	relaxationSpinBox->setDecimals(1);
	formLayout->addRow("Relaxation:", relaxationSpinBox);

	// Min Alt
	minAltSpinBox = new QDoubleSpinBox(filterGroupBox);
	minAltSpinBox->setRange(-90.0, 90.0);
	minAltSpinBox->setDecimals(1);
	minAltSpinBox->setSuffix("°");
	formLayout->addRow("Min Altitude:", minAltSpinBox);

	// Max Alt
	maxAltSpinBox = new QDoubleSpinBox(filterGroupBox);
	maxAltSpinBox->setRange(-90.0, 90.0);
	maxAltSpinBox->setDecimals(1);
	maxAltSpinBox->setSuffix("°");
	formLayout->addRow("Max Altitude:", maxAltSpinBox);

	// Moon Age (calculated, read-only) - days since new moon
	moonAgeLabel = new QLabel(filterGroupBox);
	moonAgeLabel->setText("--");
	moonAgeLabel->setMinimumHeight(25);
	formLayout->addRow("Moon Age:", moonAgeLabel);

	// Current Separation (calculated, read-only)
	currentSeparationLabel = new QLabel(filterGroupBox);
	currentSeparationLabel->setText("--");
	currentSeparationLabel->setMinimumHeight(25);
	formLayout->addRow("Current Separation:", currentSeparationLabel);

	// Color
	colorButton = new QPushButton("Choose Color", filterGroupBox);
	colorLabel = new QLabel(filterGroupBox);
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
	groupLayout->addLayout(listFormLayout);

	// Button layout for Add/Remove
	QHBoxLayout* filterButtonLayout = new QHBoxLayout();

	addButton = new QPushButton("Add Filter", filterGroupBox);
	removeButton = new QPushButton("Remove Filter", filterGroupBox);

	if (!addButton || !removeButton)
	{
		qWarning() << "MoonAvoidanceDialog: Failed to create filter buttons";
		return;
	}

	filterButtonLayout->addWidget(addButton);
	filterButtonLayout->addWidget(removeButton);
	filterButtonLayout->addStretch();

	groupLayout->addLayout(filterButtonLayout);

	// Add the group box to the main filters layout
	filtersLayout->addWidget(filterGroupBox);

	// Connect signals for filter tab
	connect(addButton, &QPushButton::clicked, this, &MoonAvoidanceDialog::addFilter);
	connect(removeButton, &QPushButton::clicked, this, &MoonAvoidanceDialog::removeFilter);

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

	// Initially disable form fields until a filter is selected
	enableFormFields(false);

	// If we have pending filters (set before dialog was created), apply them now
	if (!pendingFilters.isEmpty())
	{
		setFilters(pendingFilters);
		pendingFilters.clear();
	}
}

void MoonAvoidanceDialog::createInfoTab()
{
	infoTab = new QWidget();
	QVBoxLayout* infoLayout = new QVBoxLayout(infoTab);
	infoLayout->setContentsMargins(10, 10, 10, 10);

	infoTextBrowser = new QTextBrowser(infoTab);
	infoTextBrowser->setOpenExternalLinks(true);

	QString infoText = "<h2>Moon Avoidance Plugin</h2>"
		"<p>This plugin visualizes moon avoidance zones for astrophotography planning.</p>"
		"<h3>How it works:</h3>"
		"<ul>"
		"<li><b>Separation</b>: Base angular distance from the moon (in degrees)</li>"
		"<li><b>Width</b>: Controls how quickly avoidance decreases as moon phase changes (in days)</li>"
		"<li><b>Relaxation</b>: How much to relax avoidance when moon is low on the horizon</li>"
		"<li><b>Min/Max Altitude</b>: Altitude range where relaxation applies</li>"
		"</ul>"
		"<h3>Moon Phase Calculation:</h3>"
		"<p>The plugin uses a Lorentzian formula to calculate separation based on moon age. "
		"Avoidance is highest at full moon and decreases as the moon approaches new moon.</p>"
		"<h3>Filter Colors:</h3>"
		"<p>Different filters can be configured with different colors for easy visualization on the sky.</p>";

	infoTextBrowser->setHtml(infoText);
	infoLayout->addWidget(infoTextBrowser);
}

void MoonAvoidanceDialog::createAboutTab()
{
	aboutTab = new QWidget();
	QVBoxLayout* aboutLayout = new QVBoxLayout(aboutTab);
	aboutLayout->setContentsMargins(10, 10, 10, 10);

	aboutTextBrowser = new QTextBrowser(aboutTab);
	aboutTextBrowser->setOpenExternalLinks(true);

	QString aboutText = "<h2>About Moon Avoidance</h2>"
		"<p><b>Version:</b> 1.0.0</p>"
		"<p><b>Author:</b> Stellarium Community</p>"
		"<p><b>License:</b> GPL</p>"
		"<h3>Description:</h3>"
		"<p>This plugin implements moon avoidance calculations for astrophotography planning, "
		"compatible with NINA (Nighttime Imaging 'N' Astronomy) target scheduler logic.</p>"
		"<h3>Features:</h3>"
		"<ul>"
		"<li>Visualize moon avoidance zones on the sky</li>"
		"<li>Configure multiple filters with different avoidance parameters</li>"
		"<li>Altitude-based relaxation for low moon positions</li>"
		"<li>Real-time calculation based on moon phase and position</li>"
		"<li>Color-coded circles for easy identification</li>"
		"</ul>"
		"<h3>More Information:</h3>"
		"<p>For documentation and source code, visit the project repository.</p>";

	aboutTextBrowser->setHtml(aboutText);
	aboutLayout->addWidget(aboutTextBrowser);
}

void MoonAvoidanceDialog::createDiagramTab()
{
	diagramTab = new QWidget();
	QVBoxLayout* diagramLayout = new QVBoxLayout(diagramTab);
	diagramLayout->setContentsMargins(10, 10, 10, 10);

	QLabel* placeholderLabel = new QLabel("Diagram view coming soon...", diagramTab);
	placeholderLabel->setAlignment(Qt::AlignCenter);

	diagramLayout->addWidget(placeholderLabel);
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
